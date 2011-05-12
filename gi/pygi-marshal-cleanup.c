/* -*- Mode: C; c-basic-offset: 4 -*-
 * vim: tabstop=4 shiftwidth=4 expandtab
 *
 * Copyright (C) 2011 John (J5) Palmieri <johnp@redhat.com>, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
 * USA
 */
 
 #include "pygi-marshal-cleanup.h"
 #include <glib.h>
static inline void
_cleanup_caller_allocates (PyGIInvokeState    *state,
                           PyGIInterfaceCache *cache,
                           gpointer            data)
{
    if (cache->g_type == G_TYPE_BOXED) {
        gsize size;
        size = g_struct_info_get_size (cache->interface_info);
        g_slice_free1 (size, data);
    } else if (cache->is_foreign) {
        pygi_struct_foreign_release ((GIBaseInfo *)cache->interface_info,
                                     data);
    } else {
        g_free (data);
    }
}

/**
 * Cleanup during invoke can happen in multiple
 * stages, each of which can be the result of a
 * successful compleation of that stage or an error
 * occured which requires partial cleanup.
 *
 * For the most part, either the C interface being
 * invoked or the python object which wraps the
 * parameters, handle their lifecycles but in some
 * cases, where we have intermediate objects,
 * or when we fail processing a parameter, we need
 * to handle the clean up manually.
 *
 * There are two argument processing stages.
 * They are the in stage, where we process python
 * parameters into their C counterparts, and the out
 * stage, where we process out C parameters back
 * into python objects. The in stage also sets up
 * temporary out structures for caller allocated
 * parameters which need to be cleaned up either on
 * in stage failure or at the completion of the out
 * stage (either success or failure)
 *
 * The in stage must call one of these cleanup functions:
 *    - pygi_marshal_cleanup_args_in_marshal_success
 *       (continue to out stage)
 *    - pygi_marshal_cleanup_args_in_parameter_fail
 *       (final, exit from invoke)
 *
 * The out stage must call one of these cleanup functions which are all final:
 *    - pygi_marshal_cleanup_args_out_marshal_success
 *    - pygi_marshal_cleanup_args_return_fail
 *    - pygi_marshal_cleanup_args_out_parameter_fail
 *
 **/
void
pygi_marshal_cleanup_args_in_marshal_success (PyGIInvokeState   *state,
                                              PyGICallableCache *cache)
{

}

void
pygi_marshal_cleanup_args_invoke_success (PyGIInvokeState   *state,
                                          PyGICallableCache *cache)
{

}

void
pygi_marshal_cleanup_args_in_parameter_fail (PyGIInvokeState   *state,
                                             PyGICallableCache *cache,
                                             gssize failed_arg_index)
{

}

void
pygi_marshal_cleanup_args_return_fail (PyGIInvokeState   *state,
                                       PyGICallableCache *cache)
{

}

void
pygi_marshal_cleanup_args_out_parameter_fail (PyGIInvokeState   *state,
                                              PyGICallableCache *cache,
                                              gssize failed_out_arg_index)
{

}

void
pygi_marshal_cleanup_args (PyGIInvokeState   *state,
                           PyGICallableCache *cache,
                           gboolean invoke_failure)
{
    switch (state->stage) {
        case PYGI_INVOKE_STAGE_MARSHAL_IN_IDLE:
            /* current_arg has been marshalled so increment to start with
               next arg */
            state->current_arg++;
        case PYGI_INVOKE_STAGE_MARSHAL_IN_START:
        case PYGI_INVOKE_STAGE_NATIVE_INVOKE_FAILED:
        case PYGI_INVOKE_STAGE_NATIVE_INVOKE_DONE:
        {
            gsize i;
            /* we have not yet invoked so we only need to clean up 
               the in args and out caller allocates */

            for (i = 0; i < state->current_arg; i++) {
                PyGIArgCache *arg_cache = cache->args_cache[i];
                PyGIMarshalCleanupFunc cleanup_func = arg_cache->cleanup;

                /* FIXME: handle caller allocates */
                if (invoke_failure && 
                      arg_cache->direction == GI_DIRECTION_OUT &&
                        arg_cache->is_caller_allocates) {
                    _cleanup_caller_allocates (state,
                                               (PyGIInterfaceCache *) arg_cache,
                                               state->args[i]->v_pointer);
                } else if (cleanup_func != NULL  &&
                             arg_cache->direction != GI_DIRECTION_OUT) {
                    cleanup_func (state, arg_cache, state->args[i]->v_pointer);
                }
            }
            break;
        }

        case PYGI_INVOKE_STAGE_DONE:
        case PYGI_INVOKE_STAGE_MARSHAL_RETURN_START:
            /* clean up the return if not marshalled */
            if (cache->return_cache != NULL) {
                PyGIMarshalCleanupFunc cleanup_func =
                    cache->return_cache->cleanup;

                if (cleanup_func)
                    cleanup_func (state,
                                  cache->return_cache,
                                  state->return_arg.v_pointer);
            }

        case PYGI_INVOKE_STAGE_MARSHAL_OUT_START:
        case PYGI_INVOKE_STAGE_MARSHAL_OUT_IDLE:
        case PYGI_INVOKE_STAGE_MARSHAL_RETURN_DONE:
        {
            /* Cleanup caller allocate args and any unmarshalled arg */
            GSList *cache_item = cache->out_args;
            gsize arg_index = 0;

            if (state->stage == PYGI_INVOKE_STAGE_MARSHAL_OUT_START) {
                /* we have not yet marshalled so decrement to end with
                   previous arg */
                state->current_arg--;
            }

            /* clean up the args */
            while (cache_item != NULL) {
                PyGIArgCache *arg_cache = (PyGIArgCache *) cache_item->data;
                PyGIMarshalCleanupFunc cleanup_func = arg_cache->cleanup;

                if (arg_index > state->current_arg) {
                    if (cleanup_func != NULL)
                        cleanup_func (state,
                                      arg_cache,
                                      state->args[arg_cache->c_arg_index]->v_pointer);

                    if (arg_cache->is_caller_allocates)
                        _cleanup_caller_allocates (state,
                                                   (PyGIInterfaceCache *) arg_cache,
                                                   state->args[arg_cache->c_arg_index]->v_pointer);
                }

                arg_index++;
                cache_item = cache_item->next;
            }
            break;
        }
    }
}

void 
_pygi_marshal_cleanup_closure_unref (PyGIInvokeState *state,
                                     PyGIArgCache    *arg_cache,
                                     gpointer         data)
{
    g_closure_unref ( (GClosure *)data);
}

void
_pygi_marshal_cleanup_utf8 (PyGIInvokeState *state,
                            PyGIArgCache    *arg_cache,
                            gpointer         data)
{
    /* For in or inout values before invoke we need to free this,
     * but after invoke we we free only if transfer == GI_TRANSFER_NOTHING
     * and this is not an inout value
     *
     * For out and inout values before invoke we do nothing but after invoke
     * we free if transfer == GI_TRANSFER_EVERYTHING
     */
    switch (state->stage) {
        case PYGI_INVOKE_STAGE_MARSHAL_IN_START:
        case PYGI_INVOKE_STAGE_MARSHAL_IN_IDLE:
            g_free (data);
            break;
        case PYGI_INVOKE_STAGE_NATIVE_INVOKE_FAILED:
        case PYGI_INVOKE_STAGE_NATIVE_INVOKE_DONE:
            if (arg_cache->transfer == GI_TRANSFER_NOTHING &&
                  arg_cache->direction == GI_DIRECTION_IN)
                g_free (data);
            break;
        case PYGI_INVOKE_STAGE_MARSHAL_RETURN_START:
        case PYGI_INVOKE_STAGE_MARSHAL_RETURN_DONE:
        case PYGI_INVOKE_STAGE_MARSHAL_OUT_START:
        case PYGI_INVOKE_STAGE_MARSHAL_OUT_IDLE:
            if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
                g_free (data);
            break;
        default:
            break;
     }
}

void
_pygi_marshal_cleanup_object (PyGIInvokeState *state,
                              PyGIArgCache    *arg_cache,
                              gpointer         data)
{
    /* For in or inout values before invoke we need to unref
     * only if transfer == GI_TRANSFER_EVERYTHING
     *
     * For out and inout values before invoke we do nothing but after invoke
     * we unref if transfer == GI_TRANSFER_EVERYTHING
     */
    switch (state->stage) {
        case PYGI_INVOKE_STAGE_MARSHAL_IN_START:
        case PYGI_INVOKE_STAGE_MARSHAL_IN_IDLE:
            if (arg_cache->transfer == GI_TRANSFER_EVERYTHING &&
                  arg_cache->direction == GI_DIRECTION_IN)
                g_object_unref (G_OBJECT(data));
            break;
        case PYGI_INVOKE_STAGE_NATIVE_INVOKE_FAILED:
        case PYGI_INVOKE_STAGE_NATIVE_INVOKE_DONE:
            break;
        case PYGI_INVOKE_STAGE_MARSHAL_RETURN_START:
        case PYGI_INVOKE_STAGE_MARSHAL_RETURN_DONE:
        case PYGI_INVOKE_STAGE_MARSHAL_OUT_START:
        case PYGI_INVOKE_STAGE_MARSHAL_OUT_IDLE:
            if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
                g_object_unref (G_OBJECT(data));
            break;
        default:
            break;
     }
}

void 
_pygi_marshal_cleanup_gvalue (PyGIInvokeState *state,
                              PyGIArgCache    *arg_cache,
                              gpointer         data)
{
    /* For in or inout values before invoke we need to unset and slice_free
     * After invoke we unset and free if transfer == GI_TRANSFER_NOTHING
     *
     * For out values before invoke we do nothing but after invoke
     * we unset if transfer == GI_TRANSFER_EVERYTHING
     *
     */
    switch (state->stage) {
        case PYGI_INVOKE_STAGE_MARSHAL_IN_START:
        case PYGI_INVOKE_STAGE_MARSHAL_IN_IDLE:
                g_value_unset ((GValue *) data);
                g_slice_free (GValue, data);
            break;
        case PYGI_INVOKE_STAGE_NATIVE_INVOKE_FAILED:
        case PYGI_INVOKE_STAGE_NATIVE_INVOKE_DONE:
            break;
        case PYGI_INVOKE_STAGE_MARSHAL_RETURN_START:
        case PYGI_INVOKE_STAGE_MARSHAL_RETURN_DONE:
        case PYGI_INVOKE_STAGE_MARSHAL_OUT_START:
        case PYGI_INVOKE_STAGE_MARSHAL_OUT_IDLE:
            if (arg_cache->transfer == GI_TRANSFER_EVERYTHING)
                g_value_unset ((GValue *) data);
            break;
        default:
            break;
     }
}
