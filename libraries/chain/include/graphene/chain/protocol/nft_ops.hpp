#pragma once
#include <graphene/chain/protocol/base.hpp>
#include <graphene/chain/protocol/types.hpp>

namespace graphene { namespace chain {

   struct nft_metadata_create_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
      asset fee;

      account_id_type owner;
      std::string name;
      std::string symbol;
      std::string base_uri;

      account_id_type fee_payer()const { return owner; }
   };

   struct nft_metadata_update_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
      asset fee;

      account_id_type owner;
      nft_metadata_id_type nft_metadata_id;
      optional<std::string> name;
      optional<std::string> symbol;
      optional<std::string> base_uri;

      account_id_type fee_payer()const { return owner; }
   };

   struct nft_mint_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
      asset fee;

      account_id_type payer;

      optional<nft_metadata_id_type> nft_metadata_id;
      account_id_type owner;
      account_id_type approved;
      vector<account_id_type> approved_operators;
      std::string token_uri;

      account_id_type fee_payer()const { return payer; }
   };

   struct nft_safe_transfer_from_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
      asset fee;

      account_id_type operator_;

      account_id_type from;
      account_id_type to;
      nft_id_type token_id;
      string data;

      account_id_type fee_payer()const { return operator_; }
   };

   struct nft_approve_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
      asset fee;

      account_id_type operator_;

      account_id_type approved;
      nft_id_type token_id;

      account_id_type fee_payer()const { return operator_; }
   };

   struct nft_set_approval_for_all_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = GRAPHENE_BLOCKCHAIN_PRECISION; };
      asset fee;

      account_id_type owner;

      account_id_type operator_;
      bool approved;

      account_id_type fee_payer()const { return owner; }
   };

} } // graphene::chain

FC_REFLECT( graphene::chain::nft_metadata_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::nft_metadata_update_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::nft_mint_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::nft_safe_transfer_from_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::nft_approve_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::nft_set_approval_for_all_operation::fee_parameters_type, (fee) )

FC_REFLECT( graphene::chain::nft_metadata_create_operation, (fee) (owner) (name) (symbol) (base_uri) )
FC_REFLECT( graphene::chain::nft_metadata_update_operation, (fee) (owner) (nft_metadata_id) (name) (symbol) (base_uri) )
FC_REFLECT( graphene::chain::nft_mint_operation, (fee) (nft_metadata_id) (owner) (approved) (approved_operators) (token_uri) )
FC_REFLECT( graphene::chain::nft_safe_transfer_from_operation, (fee) (operator_) (from) (to) (token_id) (data) )
FC_REFLECT( graphene::chain::nft_approve_operation, (fee) (operator_) (approved) (token_id) )
FC_REFLECT( graphene::chain::nft_set_approval_for_all_operation, (fee) (owner) (operator_) (approved) )
