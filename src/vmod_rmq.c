#include "config.h"

#include <stdio.h>
#include <stdlib.h>

/* need vcl.h before vrt.h for vmod_evet_f typedef */
#include "vcl.h"
#include "vrt.h"
#include "cache/cache.h"

#include "vtim.h"
#include "vcc_rmq_if.h"

#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <amqp_ssl_socket.h>

const size_t infosz = 64;
char *info;

char const *exchange;
char const *routingkey;

typedef struct RMQ
{
	amqp_socket_t *s;
	amqp_connection_state_t conn;
} rmq;

/*
 * handle vmod internal state, vmod init/fini and/or varnish callback
 * (un)registration here.
 *
 * malloc'ing the info buffer is only indended as a demonstration, for any
 * real-world vmod, a fixed-sized buffer should be a global variable
 */

int __match_proto__(vmod_event_f)
	event_function(VRT_CTX, struct vmod_priv *priv, enum vcl_event_e e)
{
	char ts[VTIM_FORMAT_SIZE];
	const char *event = NULL;

	(void)ctx;
	(void)priv;

	switch (e)
	{
	case VCL_EVENT_LOAD:
		info = malloc(infosz);
		if (!info)
			return (-1);

		event = "loaded";
		break;
	case VCL_EVENT_WARM:
		event = "warmed";
		break;
	case VCL_EVENT_COLD:
		event = "cooled";
		break;
	case VCL_EVENT_DISCARD:
		free(info);
		return (0);
		break;
	default:
		return (0);
	}
	AN(event);
	VTIM_format(VTIM_real(), ts);
	snprintf(info, infosz, "vmod_rmq %s at %s", event, ts);

	return (0);
}

VCL_VOID
vmod_init(VRT_CTX, struct vmod_priv *pp, VCL_STRING host, VCL_INT port, VCL_STRING key, VCL_STRING username, VCL_STRING password)
{
	if (pp->priv == NULL)
	{
		rmq *mq = malloc(sizeof(rmq));
		mq->conn = amqp_new_connection();
		mq->s = amqp_tcp_socket_new(mq->conn);
		if (mq->s)
		{
			int status = amqp_socket_open(mq->s, host, port);
			if (status)
			{
				return;
			}
			amqp_login(mq->conn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
					   username, password);
			amqp_channel_open(mq->conn, 1);
			amqp_get_rpc_reply(mq->conn);
			amqp_queue_bind(mq->conn, 1, amqp_cstring_bytes(key),
							amqp_cstring_bytes("amq.direct"), amqp_cstring_bytes(key),
							amqp_empty_table);
		}
		pp->priv = mq;
		AN(pp->priv);
	}
}

VCL_STRING
vmod_send(VRT_CTX, struct vmod_priv *pp, VCL_STRING remote_host, VCL_STRING country, VCL_STRING geo_location, VCL_STRING type)
{
	char *p;
	unsigned v, u;
	u = WS_Reserve(ctx->ws, 0); /* Reserve some work space */
	p = ctx->ws->f;				/* Front of workspace area */
	v = snprintf(p, u, "%s|%s|%s|%s", remote_host, country, geo_location, type);

	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes("text/plain");

	props.delivery_mode = 2; /* persistent delivery mode */
	amqp_basic_publish(((rmq *)pp->priv)->conn, 1, amqp_cstring_bytes("amq.direct"),
					   amqp_cstring_bytes("test"), 1, 0,
					   &props, amqp_cstring_bytes(p));
	WS_Release(ctx->ws, v);
	return (p);
}
