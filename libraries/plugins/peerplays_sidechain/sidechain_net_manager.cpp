#include <graphene/peerplays_sidechain/sidechain_net_manager.hpp>

#include <fc/log/logger.hpp>
#include <graphene/chain/son_wallet_object.hpp>
#include <graphene/peerplays_sidechain/sidechain_net_handler_bitcoin.hpp>

namespace graphene { namespace peerplays_sidechain {

sidechain_net_manager::sidechain_net_manager(peerplays_sidechain_plugin& _plugin) :
    plugin(_plugin),
    database(_plugin.database())
{
   ilog(__FUNCTION__);
}

sidechain_net_manager::~sidechain_net_manager() {
   ilog(__FUNCTION__);
}

bool sidechain_net_manager::create_handler(peerplays_sidechain::sidechain_type sidechain, const boost::program_options::variables_map& options) {
   ilog(__FUNCTION__);

   bool ret_val = false;

   switch (sidechain) {
      case sidechain_type::bitcoin: {
          std::unique_ptr<sidechain_net_handler> h = std::unique_ptr<sidechain_net_handler>(new sidechain_net_handler_bitcoin(plugin, options));
          net_handlers.push_back(std::move(h));
          ret_val = true;
          break;
      }
      default:
         assert(false);
   }

   return ret_val;
}

signed_transaction sidechain_net_manager::recreate_primary_wallet() {
   ilog(__FUNCTION__);

   signed_transaction trx = {};

   const auto& idx = database.get_index_type<son_wallet_index>().indices().get<by_id>();
   auto swo = idx.rbegin();

   for ( size_t i = 0; i < net_handlers.size(); i++ ) {
      son_wallet_update_operation op = net_handlers.at(i)->recreate_primary_wallet();

      if (swo != idx.rend()) {
         if (op.son_wallet_id == swo->id) {
            trx.operations.push_back(op);
         }
      }
   }

   return trx;
}

string sidechain_net_manager::recreate_primary_wallet(peerplays_sidechain::sidechain_type sidechain, const vector<string>& participants) {
   ilog(__FUNCTION__);
   for ( size_t i = 0; i < net_handlers.size(); i++ ) {
      if (net_handlers.at(i)->get_sidechain() == sidechain) {
         return net_handlers.at(i)->recreate_primary_wallet(participants);
      }
   }
   return "";
}

} } // graphene::peerplays_sidechain
