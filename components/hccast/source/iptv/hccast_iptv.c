#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#include <hccast_iptv.h>
#include "hccast_iptv_api.h"

static hccast_iptv_app_instance_st g_iptv_inst[HCCAST_IPTV_APP_MAX] = {0};

static pthread_mutex_t g_iptv_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_service_init = false;

/**
 * The function `hccast_iptv_service_init` initializes IPTV services by locking a mutex, checking if
 * the service is already initialized, and initializing it if not.
 *
 * @return an integer value of 0.
 */
int hccast_iptv_service_init()
{
    pthread_mutex_lock(&g_iptv_mutex);
    if (!g_service_init)
    {
        g_service_init = true;
        memset(g_iptv_inst, 0, sizeof(hccast_iptv_app_instance_st) * HCCAST_IPTV_APP_MAX);
    }
    pthread_mutex_unlock(&g_iptv_mutex);

    return 0;
}

/**
 * The function `hccast_iptv_service_app_init` initializes an IPTV service application with a given ID.
 *
 * @param app_id The `app_id` parameter is an integer value representing the ID of the IPTV service
 * application that is being initialized.
 *
 * @return The function `hccast_iptv_service_app_init` is returning 0.
 */
int hccast_iptv_service_app_init(int app_id)
{
    int ret = -1;
    pthread_mutex_lock(&g_iptv_mutex);
    if (!hccast_iptv_api_init(app_id))
    {
        ret = hccast_iptv_api_attach(app_id);
    }
    pthread_mutex_unlock(&g_iptv_mutex);
    return 0;
}

int hccast_iptv_app_register(int app_id, hccast_iptv_app_instance_st *inst)
{
    if (app_id >= HCCAST_IPTV_APP_MAX || !inst)
    {
        return -1;
    }

    memcpy(&g_iptv_inst[app_id], inst, sizeof(hccast_iptv_app_instance_st));
    g_iptv_inst[app_id].initialized = 1;
    return 0;
}

/**
 * The function `hccast_iptv_app_open` returns a pointer to the `g_iptv_inst` array element
 * corresponding to the given `app_id` if it is within bounds and initialized.
 *
 * @param app_id The `app_id` parameter is an integer value representing the ID of the IPTV application
 * that you want to open.
 *
 * @return A pointer to the `g_iptv_inst` structure at index `app_id` is being returned.
 */
void *hccast_iptv_app_open(int app_id)
{
    if (app_id >= HCCAST_IPTV_APP_MAX || !g_iptv_inst[app_id].initialized)
    {
        return NULL;
    }

    return (void *)(&g_iptv_inst[app_id]);
}

/**
 * The function `hccast_iptv_app_init` initializes an IPTV application instance with a given
 * configuration and notifier.
 *
 * @param inst The `inst` parameter is a void pointer that points to an instance of the
 * `hccast_iptv_app_instance_st` structure.
 * @param config The `config` parameter in the `hccast_iptv_app_init` function is of type
 * `hccast_iptv_app_config_st *`, which is a pointer to a structure of type
 * `hccast_iptv_app_config_st`. This structure likely contains configuration settings or
 * @param notifier The `notifier` parameter in the `hccast_iptv_app_init` function is of type
 * `hccast_iptv_notifier`. It is a function pointer that points to a function responsible for notifying
 * events related to the IPTV application.
 *
 * @return the result of calling the `init` function of the `app_inst` instance with the provided
 * `config` and `notifier` parameters.
 */
int hccast_iptv_app_init(void *inst, const hccast_iptv_app_config_st *config, hccast_iptv_notifier notifier)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->init)
    {
        return -1;
    }

    return app_inst->init(config, notifier);
}

/**
 * The function `hccast_iptv_app_deinit` deinitialize an IPTV application instance.
 *
 * @param inst The `inst` parameter is a void pointer that points to an instance of the
 * `hccast_iptv_app_instance_st` structure.
 *
 * @return the result of calling the `deinit` function stored in the `app_inst` structure.
 */
int hccast_iptv_app_deinit(void *inst)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->deinit)
    {
        return -1;
    }

    return app_inst->deinit();
}

/**
 * This function retrieves the configuration settings for an IPTV application instance.
 *
 * @param inst A pointer to an instance of the IPTV application.
 * @param conf conf is a pointer to a structure of type hccast_iptv_app_config_st, which is used to
 * store the configuration settings for an IPTV application. The function hccast_iptv_config_get()
 * retrieves the configuration settings from the application instance pointed to by inst and stores
 * them in
 *
 * @return the result of calling the `conf_get` function of the `app_inst` object with the `conf`
 * parameter passed as an argument. The return type of the `conf_get` function is not specified in the
 * code snippet, so it is unclear what is being returned. However, the function signature suggests that
 * it should return an integer value.
 */
int hccast_iptv_config_get(void *inst, hccast_iptv_app_config_st *conf)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->conf_get)
    {
        return -1;
    }

    return app_inst->conf_get(conf);
}

/**
 * This function sets the configuration for an IPTV application instance.
 *
 * @param inst A pointer to an instance of the hccast_iptv_app_instance_st struct, which represents an
 * instance of an IPTV application.
 * @param conf A pointer to a structure of type hccast_iptv_app_config_st, which contains the
 * configuration settings for an IPTV application.
 *
 * @return the result of calling the `conf_set` function of the `app_inst` object with the `conf`
 * parameter passed to the function. The return type of the `conf_set` function is not specified in the
 * code snippet, so it is unclear what is being returned by the `hccast_iptv_config_set` function.
 * However, the function signature suggests that it
 */
int hccast_iptv_config_set(void *inst, const hccast_iptv_app_config_st *conf)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->conf_set)
    {
        return -1;
    }

    return app_inst->conf_set(conf);
}

/**
 * The function retrieves IPTV categories from an initialized app instance.
 *
 * @param inst A void pointer to an instance of the hccast_iptv_app_instance_st struct, which contains
 * information about the IPTV application instance.
 * @param cate A pointer to a pointer of hccast_iptv_category_node_st, which is a struct representing a
 * node in a linked list of IPTV categories. This function is used to retrieve the current category
 * node from the IPTV app instance.
 *
 * @return an integer value. If the function execution is successful, it will return 0. If there is an
 * error, it will return -1.
 */
hccast_iptv_ret_e hccast_iptv_cate_get(void *inst, hccast_iptv_cate_st **cate)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->cate_fetch)
    {
        return -1;
    }

    return app_inst->cate_get(cate);
}

/**
 * This function fetches IPTV categories and returns a list of nodes.
 *
 * @param inst A void pointer to an instance of the hccast_iptv_app_instance_st struct, which contains
 * information about the IPTV application instance.
 * @param req hccast_iptv_cate_req_st is a structure that contains the request parameters for fetching
 * IPTV categories. The exact contents of this structure are not shown in the code snippet provided.
 * @param list_out A pointer to a pointer of hccast_iptv_info_list_st, which is the output parameter
 * that will contain the fetched IPTV category list.
 *
 * @return an integer value. The specific value depends on the execution of the function. If the
 * function executes successfully, it will return a value of 0 or a positive integer. If there is an
 * error, it will return a negative integer.
 */
hccast_iptv_ret_e hccast_iptv_cate_fetch(void *inst, hccast_iptv_cate_req_st *req, hccast_iptv_info_list_st **list_out)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->cate_get)
    {
        return -1;
    }

    return app_inst->cate_fetch(req, list_out);
}

/**
 * This function searches for IPTV channels and returns a list of nodes.
 *
 * @param inst A void pointer to an instance of the hccast_iptv_app_instance_st struct, which contains
 * information about the application instance.
 * @param req req is a pointer to a structure of type hccast_iptv_search_req_st, which contains the
 * search request parameters.
 * @param list_out list_out is a pointer to a pointer of hccast_iptv_info_list_st, which is an output
 * parameter. The function hccast_iptv_search will populate this parameter with a linked list of search
 * results.
 *
 * @return a value of type hccast_iptv_ret_e, which is likely an enumerated type representing different
 * return codes or error states. The specific value being returned depends on the execution of the
 * function and the values of the input parameters.
 */
hccast_iptv_ret_e hccast_iptv_search_fetch(void *inst, hccast_iptv_search_req_st *req, hccast_iptv_info_list_st **list_out)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->search_fetch)
    {
        return -1;
    }

    return app_inst->search_fetch(req, list_out);
}

/**
 * This function retrieves a list of IPTV pages in a specified direction.
 *
 * @param inst A void pointer to an instance of the hccast_iptv_app_instance_st struct, which contains
 * information about the IPTV application instance.
 * @param direction an enum type variable of hccast_iptv_page_direction, which specifies the direction
 * of the page to be retrieved. It can have one of the following values:
 * @param list_out A pointer to a pointer of hccast_iptv_info_list_st, which will be used to return the
 * list of IPTV pages.
 *
 * @return an integer value, which could be either a success code or an error code. The specific
 * meaning of the return value depends on the implementation of the function and the context in which
 * it is called.
 */
hccast_iptv_ret_e hccast_iptv_page_get(void *inst, hccast_iptv_page_direction direction, hccast_iptv_info_list_st **list_out)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->page_get)
    {
        return -1;
    }

    return app_inst->page_get(direction, list_out);
}

/**
 * This function retrieves IPTV links based on a request and returns them through a pointer.
 *
 * @param inst The "inst" parameter is a void pointer to an instance of a structure
 * "hccast_iptv_app_instance_st".
 * @param req req is a pointer to a structure of type hccast_iptv_links_req_st, which contains the
 * request parameters for retrieving IPTV links.
 * @param links The "links" parameter is a double pointer to a structure of type
 * "hccast_iptv_links_node_st". This function is expected to populate this structure with the IPTV
 * links requested by the "req" parameter. The function returns an integer value indicating success or
 * failure.
 *
 * @return an integer value, which could be either a success or error code. If the function is
 * successful, it will return a value of 0 or a positive integer. If there is an error, it will return
 * a negative integer.
 */
hccast_iptv_ret_e hccast_iptv_link_fetch(void *inst, hccast_iptv_info_req_st *req, hccast_iptv_links_st **links)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->link_fetch)
    {
        return -1;
    }

    return app_inst->link_fetch(req, links);
}

/**
 * This function retrieves IPTV links based on a request and returns an error code.
 *
 * @param inst A void pointer to an instance of the hccast_iptv_app_instance_st struct, which contains
 * information about the application instance.
 * @param req A pointer to a structure of type hccast_iptv_links_req_st, which contains the request
 * parameters for retrieving IPTV links.
 * @param links The "links" parameter is a double pointer to a structure of type
 * "hccast_iptv_links_node_st". This parameter is used to return the list of IPTV links requested by
 * the user. The function "hccast_iptv_link_get" will populate this structure with the
 *
 * @return a value of type hccast_iptv_ret_e, which is likely an enumerated type representing different
 * error codes or success statuses. The specific value being returned depends on the execution of the
 * function and the values of the input parameters.
 */
hccast_iptv_ret_e hccast_iptv_link_switch(void *inst, hccast_iptv_links_req_st *req, hccast_iptv_links_st **links)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->link_switch)
    {
        return -1;
    }

    return app_inst->link_switch(req, links);
}

/**
 * This function handles abort events for an IPTV application instance.
 *
 * @param inst The "inst" parameter is a void pointer to an instance of a structure or object. In this
 * case, it is being cast to a pointer of type "hccast_iptv_app_instance_st" using a typecast.
 *
 * @return the value returned by the `app_inst->handle_abort()` function, which is of type
 * `hccast_iptv_ret_e`.
 */
hccast_iptv_ret_e hccast_iptv_handle_abort(void *inst)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->handle_abort)
    {
        return -1;
    }

    return app_inst->handle_abort();
}

hccast_iptv_ret_e hccast_iptv_handle_flush(void *inst)
{
    hccast_iptv_app_instance_st *app_inst = (hccast_iptv_app_instance_st *)inst;

    if (!app_inst || !app_inst->initialized || !app_inst->handle_flush)
    {
        return -1;
    }

    return app_inst->handle_flush();
}

