#include <ccan/array_size/array_size.h>
#include <lightningd/json.h>
#include <lightningd/notification.h>

const char *notification_topics[] = {
	"connect",
	"disconnect",
	"warning",
	"invoice_payment",
	"channel_opened"
};

bool notifications_have_topic(const char *topic)
{
	for (size_t i=0; i<ARRAY_SIZE(notification_topics); i++)
		if (streq(topic, notification_topics[i]))
			return true;
	return false;
}

void notify_connect(struct lightningd *ld, struct node_id *nodeid,
		    struct wireaddr_internal *addr)
{
	struct jsonrpc_notification *n =
	    jsonrpc_notification_start(NULL, "connect");
	json_add_node_id(n->stream, "id", nodeid);
	json_add_address_internal(n->stream, "address", addr);
	jsonrpc_notification_end(n);
	plugins_notify(ld->plugins, take(n));
}

void notify_disconnect(struct lightningd *ld, struct node_id *nodeid)
{
	struct jsonrpc_notification *n =
	    jsonrpc_notification_start(NULL, "disconnect");
	json_add_node_id(n->stream, "id", nodeid);
	jsonrpc_notification_end(n);
	plugins_notify(ld->plugins, take(n));
}

/*'warning' is based on LOG_UNUSUAL/LOG_BROKEN level log
 *(in plugin module, they're 'warn'/'error' level). */
void notify_warning(struct lightningd *ld, struct log_entry *l)
{
	struct jsonrpc_notification *n =
	    jsonrpc_notification_start(NULL, "warning");
	json_object_start(n->stream, "warning");
	/* Choose "BROKEN"/"UNUSUAL" to keep consistent with the habit
	 * of plugin. But this may confuses the users who want to 'getlog'
	 * with the level indicated by notifications. It is the duty of a
	 * plugin to eliminate this misunderstanding.
	 */
	json_add_string(n->stream, "level",
			l->level == LOG_BROKEN ? "error"
			: "warn");
	/* unsuaul/broken event is rare, plugin pay more attentions on
	 * the absolute time, like when channels failed. */
	json_add_time(n->stream, "time", l->time.ts);
	json_add_string(n->stream, "source", l->prefix);
	json_add_string(n->stream, "log", l->log);
	json_object_end(n->stream); /* .warning */
	jsonrpc_notification_end(n);
	plugins_notify(ld->plugins, take(n));
}

void notify_invoice_payment(struct lightningd *ld, struct amount_msat amount,
			    struct preimage preimage, const struct json_escape *label)
{
	struct jsonrpc_notification *n =
		jsonrpc_notification_start(NULL, "invoice_payment");
	json_object_start(n->stream, "invoice_payment");
	json_add_string(n->stream, "msat",
			type_to_string(tmpctx, struct amount_msat, &amount));
	json_add_hex(n->stream, "preimage", &preimage, sizeof(preimage));
	json_add_escaped_string(n->stream, "label", label);
	json_object_end(n->stream);
	jsonrpc_notification_end(n);
	plugins_notify(ld->plugins, take(n));
}

void notify_channel_opened(struct lightningd *ld, struct node_id *node_id,
			   struct amount_sat *funding_sat, struct bitcoin_txid *funding_txid,
			   bool *funding_locked)
{
	struct jsonrpc_notification *n =
	    jsonrpc_notification_start(NULL, "channel_opened");
	json_object_start(n->stream, "channel_opened");
	json_add_node_id(n->stream, "id", node_id);
	json_add_amount_sat_only(n->stream, "amount", *funding_sat);
	json_add_txid(n->stream, "funding_txid", funding_txid);
	json_add_bool(n->stream, "funding_locked", funding_locked);
	json_object_end(n->stream);
	jsonrpc_notification_end(n);
	plugins_notify(ld->plugins, take(n));
}
