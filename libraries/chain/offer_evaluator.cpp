#include <graphene/chain/offer_evaluator.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/offer_object.hpp>
#include <graphene/chain/nft_object.hpp>
#include <graphene/chain/exceptions.hpp>
#include <graphene/chain/hardfork.hpp>
#include <graphene/chain/is_authorized_asset.hpp>
#include <iostream>

namespace graphene
{
    namespace chain
    {

        void_result offer_evaluator::do_evaluate(const offer_operation &op)
        {
            try
            {
                const database &d = db();
                op.issuer(d);
                for (const auto &item : op.item_ids)
                {
                    const auto &nft_obj = item(d);
                    FC_ASSERT(!d.item_locked(item), "Item(s) is already on sale");
                    bool is_owner = (nft_obj.owner == op.issuer);
                    bool is_approved = (nft_obj.approved == op.issuer);
                    bool is_approved_operator = (std::find(nft_obj.approved_operators.begin(), nft_obj.approved_operators.end(), op.issuer) != nft_obj.approved_operators.end());
                    if (op.buying_item)
                    {
                        FC_ASSERT(!is_owner, "Buyer cannot already be an onwer of the item");
                        FC_ASSERT(!is_approved, "Buyer cannot already be approved account of the item");
                        FC_ASSERT(!is_approved_operator, "Buyer cannot already be an approved operator of the item");
                    }
                    else
                    {
                        FC_ASSERT(is_owner || is_approved || is_approved_operator, "Issuer has no authority to sell the item");
                    }
                }
                FC_ASSERT(op.offer_expiration_date > d.head_block_time(), "Expiration should be in future");
                FC_ASSERT(op.fee.amount >= 0, "Invalid fee");
                FC_ASSERT(op.minimum_price.amount >= 0 && op.maximum_price.amount > 0, "Invalid amount");
                FC_ASSERT(op.minimum_price.asset_id == op.maximum_price.asset_id, "Asset ID mismatch");
                FC_ASSERT(op.maximum_price >= op.minimum_price, "Invalid max min prices");
                return void_result();
            }
            FC_CAPTURE_AND_RETHROW((op))
        }

        object_id_type offer_evaluator::do_apply(const offer_operation &op)
        {
            try
            {
                database &d = db();
                if (op.buying_item)
                {
                    d.adjust_balance(op.issuer, -op.maximum_price);
                }

                const auto &offer_obj = db().create<offer_object>([&](offer_object &obj) {
                    obj.issuer = op.issuer;

                    obj.item_ids = op.item_ids;

                    obj.minimum_price = op.minimum_price;
                    obj.maximum_price = op.maximum_price;

                    obj.buying_item = op.buying_item;
                    obj.offer_expiration_date = op.offer_expiration_date;
                });
                return offer_obj.id;
            }
            FC_CAPTURE_AND_RETHROW((op))
        }

        void_result bid_evaluator::do_evaluate(const bid_operation &op)
        {
            try
            {
                const database &d = db();
                const auto &offer = op.offer_id(d);
                op.bidder(d);
                for (const auto &item : offer.item_ids)
                {
                    const auto &nft_obj = item(d);
                    bool is_owner = (nft_obj.owner == op.bidder);
                    bool is_approved = (nft_obj.approved == op.bidder);
                    bool is_approved_operator = (std::find(nft_obj.approved_operators.begin(), nft_obj.approved_operators.end(), op.bidder) != nft_obj.approved_operators.end());
                    if (offer.buying_item)
                    {
                        FC_ASSERT(is_owner || is_approved || is_approved_operator, "Bidder has no authority to sell the item");
                    }
                    else
                    {
                        FC_ASSERT(!is_owner, "Bidder cannot already be an onwer of the item");
                        FC_ASSERT(!is_approved, "Bidder cannot already be an approved account of the item");
                        FC_ASSERT(!is_approved_operator, "Bidder cannot already be an approved operator of the item");
                    }
                }

                FC_ASSERT(op.bid_price.asset_id == offer.minimum_price.asset_id, "Asset type mismatch");
                FC_ASSERT(offer.minimum_price.amount == 0 || op.bid_price >= offer.minimum_price);
                FC_ASSERT(offer.maximum_price.amount == 0 || op.bid_price <= offer.maximum_price);
                if (offer.bidder)
                {
                    FC_ASSERT((offer.buying_item && op.bid_price < *offer.bid_price) || (!offer.buying_item && op.bid_price > *offer.bid_price), "There is already a better bid than this");
                }
                return void_result();
            }
            FC_CAPTURE_AND_RETHROW((op))
        }

        void_result bid_evaluator::do_apply(const bid_operation &op)
        {
            try
            {
                database &d = db();

                const auto &offer = op.offer_id(d);

                if (!offer.buying_item)
                {
                    if (offer.bidder)
                    {
                        d.adjust_balance(*offer.bidder, *offer.bid_price);
                    }
                    d.adjust_balance(op.bidder, -op.bid_price);
                }
                d.modify(op.offer_id(d), [&](offer_object &o) {
                    if (op.bid_price == (offer.buying_item ? offer.minimum_price : offer.maximum_price))
                    {
                        o.offer_expiration_date = d.head_block_time();
                    }
                    o.bidder = op.bidder;
                    o.bid_price = op.bid_price;
                });
                return void_result();
            }
            FC_CAPTURE_AND_RETHROW((op))
        }

        void_result finalize_offer_evaluator::do_evaluate(const finalize_offer_operation &op)
        {
            try
            {
                const database &d = db();
                const auto &offer = op.offer_id(d);

                if (op.result != result_type::ExpiredNoBid)
                {
                    FC_ASSERT(offer.bidder, "No valid bidder");
                    FC_ASSERT((*offer.bid_price).amount >= 0, "Invalid bid price");
                }
                else
                {
                    FC_ASSERT(!offer.bidder, "There should not be a valid bidder");
                }

                switch (op.result)
                {
                case result_type::Expired:
                case result_type::ExpiredNoBid:
                    FC_ASSERT(offer.offer_expiration_date <= d.head_block_time(), "Offer finalized beyong expiration time");
                    break;
                default:
                    FC_THROW_EXCEPTION(fc::assert_exception, "finalize_offer_operation: unknown result type.");
                    break;
                }

                return void_result();
            }
            FC_CAPTURE_AND_RETHROW((op))
        }

        void_result finalize_offer_evaluator::do_apply(const finalize_offer_operation &op)
        {
            try
            {
                database &d = db();

                offer_object offer = op.offer_id(d);
                vector<nft_safe_transfer_from_operation> xfer_ops;

                if (op.result != result_type::ExpiredNoBid)
                {
                    if (offer.buying_item)
                    {
                        d.adjust_balance(*offer.bidder, *offer.bid_price);
                        if (offer.bid_price < offer.maximum_price)
                        {
                            d.adjust_balance(offer.issuer, offer.maximum_price - *offer.bid_price);
                        }
                    }
                    else
                    {
                        d.adjust_balance(offer.issuer, *offer.bid_price);
                    }

                    for (const auto &item : offer.item_ids)
                    {
                        const auto &nft_obj = item(d);
                        nft_safe_transfer_from_operation xfer_op;
                        xfer_op.from = nft_obj.owner;
                        xfer_op.token_id = item;
                        xfer_op.fee = asset(0, asset_id_type());
                        if (offer.buying_item)
                        {
                            xfer_op.to = offer.issuer;
                            xfer_op.operator_ = *offer.bidder;
                        }
                        else
                        {
                            xfer_op.to = *offer.bidder;
                            xfer_op.operator_ = offer.issuer;
                        }
                        xfer_ops.push_back(std::move(xfer_op));
                    }
                }
                else
                {
                    if (offer.buying_item)
                    {
                        d.adjust_balance(offer.issuer, offer.maximum_price);
                    }
                }
                d.create<offer_history_object>([&](offer_history_object &obj) {
                    obj.issuer = offer.issuer;
                    obj.item_ids = offer.item_ids;
                    obj.bidder = offer.bidder;
                    obj.bid_price = offer.bid_price;
                    obj.minimum_price = offer.minimum_price;
                    obj.maximum_price = offer.maximum_price;
                    obj.buying_item = offer.buying_item;
                    obj.offer_expiration_date = offer.offer_expiration_date;
                });
                // This should unlock the item
                d.remove(op.offer_id(d));
                // Send safe transfer from ops
                if (xfer_ops.size() > 0)
                {
                    transaction_evaluation_state xfer_context(&d);
                    xfer_context.skip_fee_schedule_check = true;
                    for (const auto &xfer_op : xfer_ops)
                    {
                        d.apply_operation(xfer_context, xfer_op);
                    }
                }

                return void_result();
            }
            FC_CAPTURE_AND_RETHROW((op))
        }
    } // namespace chain
} // namespace graphene