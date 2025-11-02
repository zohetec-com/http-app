#include <jgb/core.h>
#include <jgb/log.h>
#include <jgb/helper.h>
#include <microhttpd.h>

//static const char* resp_str = "hello, http";

static enum MHD_Result
request_dispatch (void *cls,
         struct MHD_Connection *connection,
         const char *url,
         const char *method,
         const char *version,
         const char *upload_data,
         size_t *upload_data_size,
         void **req_cls)
{
    static int aptr;
    struct MHD_Response *response;
    enum MHD_Result ret;

    (void) cls;
    (void) url;               /* Unused. Silent compiler warning. */
    (void) version;           /* Unused. Silent compiler warning. */
    (void) upload_data;       /* Unused. Silent compiler warning. */
    (void) upload_data_size;  /* Unused. Silent compiler warning. */

    jgb_debug("{ method = %s, url = %s }", method, url);

    if (0 != strcmp (method, "GET"))
        return MHD_NO;              /* unexpected method */

    if (&aptr != *req_cls)
    {
        /* do never respond on first call */
        *req_cls = &aptr;
        return MHD_YES;
    }
    *req_cls = NULL;                  /* reset when done */

    const char* s = url;
    const char* e;
    int r;
    std::string resp_text = "not found";

    r = jgb::url_get_part(&s, &e);
    if(!r)
    {
        if(!strncmp("conf", s, e - s))
        {
            const char* path = e;
            jgb::config* conf;
            r = jgb::core::get_instance()->root_conf()->get(path, &conf);
            if(!r)
            {
                resp_text = conf->to_string();
            }
        }
    }

    response =
        MHD_create_response_from_buffer (resp_text.length(),
                                               (void*) resp_text.c_str(), MHD_RESPMEM_MUST_COPY);
    ret = MHD_queue_response (connection,
                             MHD_HTTP_OK,
                             response);
    MHD_destroy_response (response);
    return ret;
}

static struct MHD_Daemon *d;
static int tsk_init(void* worker)
{
    jgb::worker* w = (jgb::worker*) worker;
    jgb::config* conf = w->get_config();
    int port;
    int timeout = 60;
    conf->get("port", port);
    conf->get("timeout", timeout);
    jgb_info("httpd start. { port = %d, timeout = %u }", port, timeout);
    d = MHD_start_daemon (
                         MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD | MHD_USE_ERROR_LOG,
                         (uint16_t) port,
                         NULL, NULL, request_dispatch, NULL,
                         MHD_OPTION_CONNECTION_TIMEOUT, timeout,
                         MHD_OPTION_END);
    if (d)
    {
        return 0;
    }
    jgb_fail("httpd start failed.");
    return JGB_ERR_FAIL;
}

static void tsk_exit(void*)
{
    MHD_stop_daemon (d);
}

static jgb_loop_t loop
{
    .setup = tsk_init,
    .loops = nullptr,
    .exit = tsk_exit
};

jgb_api_t httpd
{
    .version = MAKE_API_VERSION(0, 1),
    .desc = "template app",
    .init = nullptr,
    .release = nullptr,
    .create = nullptr,
    .destroy = nullptr,
    .commit = nullptr,
    .loop = &loop
};
