#include "amulet.h"

static void bind_attribute_array(am_render_state *rstate,
    am_gluint location, am_attribute_client_type type, int dims, am_buffer_view *view)
{
    am_set_attribute_array_enabled(location, true);
    am_buffer *buf = view->buffer;
    if (buf->arraybuf_id == 0) buf->create_arraybuf();
    buf->update_if_dirty();
    am_bind_buffer(AM_ARRAY_BUFFER, buf->arraybuf_id);
    am_set_attribute_pointer(location, dims, type, view->normalized, view->stride, view->offset);
    if (view->size < rstate->max_draw_array_size) {
        rstate->max_draw_array_size = view->size;
    }
}

static void bind_sampler2d(am_render_state *rstate,
    am_gluint location, int texture_unit, am_texture2d *texture)
{
    if (texture->buffer != NULL) {
        texture->buffer->update_if_dirty();
    }
    am_set_active_texture_unit(texture_unit);
    am_bind_texture(AM_TEXTURE_BIND_TARGET_2D, texture->texture_id);
    am_set_uniform1i(location, texture_unit);
}

static void report_incompatible_param_type(am_program_param *param) {
    am_program_param_name_slot *slot = &am_param_name_map[param->name];
    const char *shader_type;
    const char *client_type;
    shader_type = am_program_param_type_name(param->type);
    client_type = am_program_param_client_type_name(slot);
    const char *client_type_prefix = " ";
    if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY ||
        slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED)
    {
        client_type_prefix = "n ";
    }
    if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED) {
        am_log1("WARNING: %s '%s' was not bound to anything",
            shader_type, slot->name);
    } else {
        am_log1("WARNING: ignoring incompatible binding of %s '%s' to a%s%s",
            shader_type, slot->name, client_type_prefix, client_type);
    }
}

void am_program_param::bind(am_render_state *rstate) {
    am_program_param_name_slot *slot = &am_param_name_map[name];
    bool bound = false;
    switch (type) {
        case AM_PROGRAM_PARAM_UNIFORM_1F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_1F) {
                am_set_uniform1f(location, slot->value.value.f);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_2F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_2F) {
                am_set_uniform2f(location, slot->value.value.v2);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_3F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_3F) {
                am_set_uniform3f(location, slot->value.value.v3);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_4F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_4F) {
                am_set_uniform4f(location, slot->value.value.v4);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_MAT2:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2) {
                am_set_uniform_mat2(location, slot->value.value.m2);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_MAT3:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3) {
                am_set_uniform_mat3(location, slot->value.value.m3);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_MAT4:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4) {
                am_set_uniform_mat4(location, slot->value.value.m4);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_UNIFORM_SAMPLER2D:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D) {
                bind_sampler2d(rstate, location, slot->value.value.sampler2d.texture_unit, slot->value.value.sampler2d.texture);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_ATTRIBUTE_1F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY && slot->value.value.arr->type == AM_VIEW_TYPE_FLOAT) {
                bind_attribute_array(rstate, location, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT, 1, slot->value.value.arr);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_ATTRIBUTE_2F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY && slot->value.value.arr->type == AM_VIEW_TYPE_FLOAT2) {
                bind_attribute_array(rstate, location, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT, 2, slot->value.value.arr);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_ATTRIBUTE_3F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY && slot->value.value.arr->type == AM_VIEW_TYPE_FLOAT3) {
                bind_attribute_array(rstate, location, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT, 3, slot->value.value.arr);
                bound = true;
            }
            break;
        case AM_PROGRAM_PARAM_ATTRIBUTE_4F:
            if (slot->value.type == AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY && slot->value.value.arr->type == AM_VIEW_TYPE_FLOAT4) {
                bind_attribute_array(rstate, location, AM_ATTRIBUTE_CLIENT_TYPE_FLOAT, 4, slot->value.value.arr);
                bound = true;
            }
            break;
    }
    if (!bound) report_incompatible_param_type(this);
}

static int am_param_name_map_capacity = 0;

am_program_param_name_slot *am_param_name_map = NULL;

void am_init_param_name_map(lua_State *L) {
    if (am_param_name_map != NULL) free(am_param_name_map);
    am_param_name_map_capacity = 32;
    am_param_name_map = (am_program_param_name_slot*)malloc(sizeof(am_program_param_name_slot) * am_param_name_map_capacity);
    for (int i = 0; i < am_param_name_map_capacity; i++) {
        am_param_name_map[i].name = NULL;
        am_param_name_map[i].value.type = AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED;
    }
    lua_newtable(L);
    lua_rawseti(L, LUA_REGISTRYINDEX, AM_PARAM_NAME_STRING_TABLE);
}

am_param_name_id am_lookup_param_name(lua_State *L, int name_idx) {
    name_idx = am_absindex(L, name_idx);
    lua_rawgeti(L, LUA_REGISTRYINDEX, AM_PARAM_NAME_STRING_TABLE);
    int strt_idx = lua_gettop(L);
    lua_pushvalue(L, name_idx);
    lua_rawget(L, strt_idx);
    if (lua_isnil(L, -1)) {
        // param name not seen before, register it.
        lua_pop(L, 1); // nil
        lua_pushvalue(L, name_idx);
        int name_ref = luaL_ref(L, strt_idx);
        lua_pushvalue(L, name_idx);
        lua_pushinteger(L, name_ref);
        lua_rawset(L, strt_idx);
        lua_pop(L, 1); // string table
        if (name_ref >= am_param_name_map_capacity) {
            int old_capacity = am_param_name_map_capacity;
            while (name_ref >= am_param_name_map_capacity) {
                am_param_name_map_capacity *= 2;
            }
            am_param_name_map = (am_program_param_name_slot*)realloc(am_param_name_map, sizeof(am_program_param_name_slot) * am_param_name_map_capacity);
            for (int i = old_capacity; i < am_param_name_map_capacity; i++) {
                am_param_name_map[i].name = NULL;
                am_param_name_map[i].value.type = AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED;
            }
        }
        am_param_name_map[name_ref].name = lua_tostring(L, name_idx);
        return name_ref;
    } else {
        int name_ref = lua_tointeger(L, -1);
        lua_pop(L, 2); // name ref, string table
        return name_ref;
    }
}

am_shader_id load_shader(lua_State *L, am_shader_type type, const char *src) {
    am_shader_id shader = am_create_shader(type);
    if (shader == 0) {
        lua_pushstring(L, "unable to create new shader");
        return 0;
    }

    char *msg = NULL;
    char *line_str = NULL;
    int line_no = -1;
    bool compiled = am_compile_shader(shader, type, src, &msg, &line_no, &line_str);
    if (!compiled) {
        assert(msg != NULL);
        const char *type_str = "<unknown>";
        switch (type) {
            case AM_VERTEX_SHADER: type_str = "vertex"; break;
            case AM_FRAGMENT_SHADER: type_str = "fragment"; break;
        }
        if (line_str != NULL && line_no > 0) {
            const char *nl = "";
            if (strlen(msg) > 0 && msg[strlen(msg)-1] != '\n') nl = "\n";
            lua_pushfstring(L, "%s shader compilation error:\n%s%sline %d:[[%s]]", type_str, msg, nl, line_no, line_str);
            free((void*)line_str);
        } else {
            lua_pushfstring(L, "%s shader compilation error:\n%s", type_str, msg);
        }
        free((void*)msg);
        am_delete_shader(shader);
        return 0;
    } else {
        assert(msg == NULL);
    }

   return shader;
}

static int create_program(lua_State *L) {
    if (!am_gl_is_initialized()) {
        return luaL_error(L, "you need to create a window before creating a shader program");
    }
    am_check_nargs(L, 2);
    const char *vertex_shader_src = lua_tostring(L, 1);
    const char *fragment_shader_src = lua_tostring(L, 2);
    if (vertex_shader_src == NULL) {
        return luaL_error(L, "expecting vertex shader source string in position 1");
    }
    if (fragment_shader_src == NULL) {
        return luaL_error(L, "expecting fragment shader source string in position 2");
    }

    am_shader_id vertex_shader = load_shader(L, AM_VERTEX_SHADER, vertex_shader_src);
    if (vertex_shader == 0) {
        return luaL_error(L, lua_tostring(L, -1));
    }
    am_shader_id fragment_shader = load_shader(L, AM_FRAGMENT_SHADER, fragment_shader_src);
    if (fragment_shader == 0) {
        am_delete_shader(vertex_shader);
        return luaL_error(L, lua_tostring(L, -1));
    }

    am_program_id program = am_create_program();
    if (program == 0) {
        am_delete_shader(vertex_shader);
        am_delete_shader(fragment_shader);
        return luaL_error(L, "unable to create shader program");
    }

    am_attach_shader(program, vertex_shader);
    am_attach_shader(program, fragment_shader);
    bool linked = am_link_program(program);

    am_delete_shader(vertex_shader);
    am_delete_shader(fragment_shader);

    if (!linked) {
        char *msg = am_get_program_info_log(program);
        lua_pushfstring(L, "shader program link error:\n%s", msg);
        free(msg);
        am_delete_program(program);
        return luaL_error(L, lua_tostring(L, -1));
    }

    int num_attributes = am_get_program_active_attributes(program);
    int num_uniforms = am_get_program_active_uniforms(program);

    int num_params = num_attributes + num_uniforms;    

    am_program_param *params = (am_program_param*)malloc(sizeof(am_program_param) * num_params);

    // Generate attribute params
    int i = 0;
    for (int index = 0; index < num_attributes; index++) {
        char *name_str;
        am_attribute_var_type type;
        int size;
        am_get_active_attribute(program, index, &name_str, &type, &size);
        lua_pushstring(L, name_str);
        int name = am_lookup_param_name(L, -1);
        lua_pop(L, 1); // name str
        am_program_param *param = &params[i];
        param->location = am_get_attribute_location(program, name_str);
        param->name = name;
        switch (type) {
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT:
                param->type = AM_PROGRAM_PARAM_ATTRIBUTE_1F;
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC2:
                param->type = AM_PROGRAM_PARAM_ATTRIBUTE_2F;
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC3:
                param->type = AM_PROGRAM_PARAM_ATTRIBUTE_3F;
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_VEC4:
                param->type = AM_PROGRAM_PARAM_ATTRIBUTE_4F;
                break;
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT2:
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT3:
            case AM_ATTRIBUTE_VAR_TYPE_FLOAT_MAT4:
            case AM_ATTRIBUTE_VAR_TYPE_UNKNOWN:
                am_log(L, 1, false, "WARNING: ignoring attribute '%s' with unsupported type", name_str);
                num_params--;
                free(name_str);
                continue;
        }
        free(name_str);
        i++;
    }

    // Generate uniform params
    for (int index = 0; index < num_uniforms; index++) {
        char *name_str;
        am_uniform_var_type type;
        int size;
        am_get_active_uniform(program, index, &name_str, &type, &size);
        lua_pushstring(L, name_str);
        int name = am_lookup_param_name(L, -1);
        lua_pop(L, 1); // name
        am_program_param *param = &params[i];
        param->location = am_get_uniform_location(program, name_str);
        param->name = name;
        switch (type) {
            case AM_UNIFORM_VAR_TYPE_FLOAT:
                param->type = AM_PROGRAM_PARAM_UNIFORM_1F;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_VEC2:
                param->type = AM_PROGRAM_PARAM_UNIFORM_2F;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_VEC3:
                param->type = AM_PROGRAM_PARAM_UNIFORM_3F;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_VEC4:
                param->type = AM_PROGRAM_PARAM_UNIFORM_4F;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_MAT2:
                param->type = AM_PROGRAM_PARAM_UNIFORM_MAT2;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_MAT3:
                param->type = AM_PROGRAM_PARAM_UNIFORM_MAT3;
                break;
            case AM_UNIFORM_VAR_TYPE_FLOAT_MAT4:
                param->type = AM_PROGRAM_PARAM_UNIFORM_MAT4;
                break;
            case AM_UNIFORM_VAR_TYPE_SAMPLER_2D:
                param->type = AM_PROGRAM_PARAM_UNIFORM_SAMPLER2D;
                break;
            case AM_UNIFORM_VAR_TYPE_INT:
            case AM_UNIFORM_VAR_TYPE_INT_VEC2:
            case AM_UNIFORM_VAR_TYPE_INT_VEC3:
            case AM_UNIFORM_VAR_TYPE_INT_VEC4:
            case AM_UNIFORM_VAR_TYPE_BOOL:
            case AM_UNIFORM_VAR_TYPE_BOOL_VEC2:
            case AM_UNIFORM_VAR_TYPE_BOOL_VEC3:
            case AM_UNIFORM_VAR_TYPE_BOOL_VEC4:
            case AM_UNIFORM_VAR_TYPE_SAMPLER_CUBE:
            case AM_UNIFORM_VAR_TYPE_UNKNOWN:
                am_log(L, 1, false, "WARNING: ignoring uniform '%s' with unsupported type", name_str);
                num_params--;
                free(name_str);
                continue;
        }
        free(name_str);
        i++;
    }

    am_program *prog = am_new_userdata(L, am_program);
    prog->program_id = program;
    prog->num_params = num_params;
    prog->sets_point_size = (strstr(vertex_shader_src, "gl_PointSize") != NULL);
    prog->params = params;

    return 1;
}

static int gc_program(lua_State *L) {
    am_program *prog = (am_program*)lua_touserdata(L, 1);
    am_delete_program(prog->program_id);
    free(prog->params);
    return 0;
}

static void register_program_mt(lua_State *L) {
    lua_newtable(L);
    am_set_default_index_func(L);
    am_set_default_newindex_func(L);
    lua_pushcclosure(L, gc_program, 0);
    lua_setfield(L, -2, "__gc");
    am_register_metatable(L, "program", MT_am_program, 0);
}

int am_create_program_node(lua_State *L) {
    am_check_nargs(L, 2);
    am_program *prog = am_get_userdata(L, am_program, 2);
    am_program_node *node = am_new_userdata(L, am_program_node);
    am_set_scene_node_child(L, node);
    node->program = prog;
    node->program_ref = node->ref(L, 2);
    return 1;
}

static void get_program(lua_State *L, void *obj) {
    am_program_node *node = (am_program_node*)obj;
    node->program->push(L);
}

static void set_program(lua_State *L, void *obj) {
    am_program_node *node = (am_program_node*)obj;
    node->program = am_get_userdata(L, am_program, 3);
    node->reref(L, node->program_ref, 3);
}

static am_property program_property = {get_program, set_program};

static void register_program_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_property(L, "program", &program_property);

    am_register_metatable(L, "program_node", MT_am_program_node, MT_am_scene_node);
}

void am_program_node::render(am_render_state *rstate) {
    am_program* old_program = rstate->active_program;
    rstate->active_program = program;
    render_children(rstate);
    rstate->active_program = old_program;
}

static void set_param_value(am_program_param_value *param, am_program_param_bind_value *val) {
    param->type = val->type;
    switch (val->type) {
        case AM_PROGRAM_PARAM_CLIENT_TYPE_1F:
            param->value.f = val->value.f;
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_2F:
            memcpy(&param->value.v2[0], glm::value_ptr(val->value.v2->v), 2 * sizeof(float));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_3F:
            memcpy(&param->value.v3[0], glm::value_ptr(val->value.v3->v), 3 * sizeof(float));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_4F:
            memcpy(&param->value.v4[0], glm::value_ptr(val->value.v4->v), 4 * sizeof(float));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2:
            memcpy(&param->value.m2[0], glm::value_ptr(val->value.m2->m), 4 * sizeof(float));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3:
            memcpy(&param->value.m3[0], glm::value_ptr(val->value.m3->m), 9 * sizeof(float));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4:
            memcpy(&param->value.m4[0], glm::value_ptr(val->value.m4->m), 16 * sizeof(float));
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY:
            param->value.arr = val->value.arr;
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D:
            param->value.sampler2d.texture = val->value.texture;
            break;
        case AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED:
            break;
    }
}

void am_bind_node::render(am_render_state *rstate) {
    am_program_param_value *old_vals = (am_program_param_value*)alloca(sizeof(am_program_param_value) * num_params);
    for (int i = 0; i < num_params; i++) {
        am_program_param_value *param = &am_param_name_map[values[i].name].value;
        old_vals[i] = *param;
        set_param_value(param, &values[i]);
        // assign texture unit if binding a sampler2D
        // XXX should this be moved into renderer.cpp?
        if (values[i].type == AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D) {
            if (old_vals[i].type != AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D) {
                param->value.sampler2d.texture_unit = rstate->next_free_texture_unit++;
            }
        }
    }
    render_children(rstate);
    for (int i = 0; i < num_params; i++) {
        am_program_param_value *param = &am_param_name_map[values[i].name].value;
        // release texture unit
        if (values[i].type == AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D) {
            if (old_vals[i].type != AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D) {
                rstate->next_free_texture_unit--;
            }
        }
        *param = old_vals[i];
    }
}

static void get_bind_node_value(lua_State *L, void *obj) {
    am_bind_node *node = (am_bind_node*)obj;
    node->pushuservalue(L);
    lua_pushlightuserdata(L, (void*)lua_tostring(L, 2));
    lua_rawget(L, -2);
    int index = lua_tointeger(L, -1);
    lua_pop(L, 2); // index, uservalue
    am_program_param_bind_value *param = &node->values[index];
    node->pushref(L, param->ref);
}

static void set_bind_node_value_common(lua_State *L, am_bind_node *node, am_program_param_bind_value *param, int idx) {
    if (param->ref != LUA_NOREF) {
        node->unref(L, param->ref);
        param->ref = LUA_NOREF;
    }
    switch (am_get_type(L, idx)) {
        case LUA_TNUMBER:
            param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_1F;
            param->value.f = lua_tonumber(L, idx);
            break;
        case MT_am_vec2:
            param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_2F;
            param->value.v2 = am_get_userdata(L, am_vec2, idx);
            break;
        case MT_am_vec3:
            param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_3F;
            param->value.v3 = am_get_userdata(L, am_vec3, idx);
            break;
        case MT_am_vec4:
            param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_4F;
            param->value.v4 = am_get_userdata(L, am_vec4, idx);
            break;
        case MT_am_mat2:
            param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2;
            param->value.m2 = am_get_userdata(L, am_mat2, idx);
            break;
        case MT_am_mat3:
            param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3;
            param->value.m3 = am_get_userdata(L, am_mat3, idx);
            break;
        case MT_am_mat4:
            param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4;
            param->value.m4 = am_get_userdata(L, am_mat4, idx);
            break;
        case MT_am_buffer_view:
            param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY;
            param->value.arr = am_get_userdata(L, am_buffer_view, idx);
            break;
        case MT_am_texture2d:
            param->type = AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D;
            param->value.texture = am_get_userdata(L, am_texture2d, idx);
            break;
        default:
            luaL_error(L, "invalid bind value of type %s", am_get_typename(L, am_get_type(L, idx)));
    }
    param->ref = node->ref(L, idx);
}

static void set_bind_node_value(lua_State *L, void *obj) {
    am_bind_node *node = (am_bind_node*)obj;
    node->pushuservalue(L);
    lua_pushlightuserdata(L, (void*)lua_tostring(L, 2));
    lua_rawget(L, -2);
    assert(lua_type(L, -1) == LUA_TNUMBER);
    int index = lua_tointeger(L, -1);
    lua_pop(L, 2); // index, uservalue
    am_program_param_bind_value *param = &node->values[index];
    set_bind_node_value_common(L, node, param, 3);
}

static am_property bind_node_value_property =
    {get_bind_node_value, set_bind_node_value};

static am_bind_node *new_bind_node(lua_State *L, int num_params) {
    // allocate extra space for the shader paramter names, values and refs
    am_bind_node *node = (am_bind_node*)am_set_metatable(L,
        new (lua_newuserdata(L, 
            sizeof(am_bind_node)
            + sizeof(am_program_param_bind_value) * num_params
            ))
        am_bind_node(), MT_am_bind_node);
    node->num_params = num_params;
    node->values = (am_program_param_bind_value*)(((uint8_t*)node) + sizeof(am_bind_node));
    for (int i = 0; i < num_params; i++) {
        node->values[i].ref = LUA_NOREF;
    }
    return node;
}

static void set_bind_property(lua_State *L, am_bind_node *node, int index, int name_idx) {
    name_idx = am_absindex(L, name_idx);
    node->pushuservalue(L);
    lua_pushvalue(L, name_idx); // param name
    lua_pushlightuserdata(L, (void*)&bind_node_value_property);
    lua_rawset(L, -3);
    lua_pushlightuserdata(L, (void*)lua_tostring(L, name_idx));
    lua_pushinteger(L, index);
    lua_rawset(L, -3);
    lua_pop(L, 1); // uservalue table
}

int am_create_bind_node(lua_State *L) {
    am_check_nargs(L, 2);
    am_bind_node *node;
    if (lua_isstring(L, 2)) {
        node = new_bind_node(L, 1);
        node->values[0].name = am_lookup_param_name(L, 2);
        set_bind_node_value_common(L, node, &node->values[0], 3);
        set_bind_property(L, node, 0, 2);
    } else if (lua_istable(L, 2)) {
        int num_params = 0;
        lua_pushnil(L);
        while (lua_next(L, 2)) {
            if (!lua_isstring(L, -2)) {
                return luaL_error(L, "all bind param names must be strings");
            }
            num_params++;
            lua_pop(L, 1);
        }
        node = new_bind_node(L, num_params);
        lua_pushnil(L);
        int index = 0;
        while (lua_next(L, 2)) {
            node->values[index].name = am_lookup_param_name(L, -2);
            set_bind_node_value_common(L, node, &node->values[index], -1);
            set_bind_property(L, node, index, -2);
            index++;
            lua_pop(L, 1);
        }
    } else {
        return luaL_error(L, "expecting a table or string in position 2");
    }

    am_set_scene_node_child(L, node);

    return 1;
}

static void register_bind_node_mt(lua_State *L) {
    lua_newtable(L);
    lua_pushcclosure(L, am_scene_node_index, 0);
    lua_setfield(L, -2, "__index");
    lua_pushcclosure(L, am_scene_node_newindex, 0);
    lua_setfield(L, -2, "__newindex");

    am_register_metatable(L, "bind", MT_am_bind_node, MT_am_scene_node);
}

#define AM_READ_MAT_NODE_IMPL(D)                                        \
void am_read_mat##D##_node::render(am_render_state *rstate) {           \
    am_program_param_value *param = &am_param_name_map[name].value;     \
    if (param->type == AM_PROGRAM_PARAM_CLIENT_TYPE_MAT##D) {           \
        m = glm::make_mat##D(&param->value.m##D[0]);                    \
    }                                                                   \
    render_children(rstate);                                            \
}                                                                       \
int am_create_read_mat##D##_node(lua_State *L) {                        \
    am_check_nargs(L, 2);                                               \
    if (!lua_isstring(L, 2)) return luaL_error(L, "expecting a string in position 2"); \
    am_read_mat##D##_node *node = am_new_userdata(L, am_read_mat##D##_node); \
    am_set_scene_node_child(L, node);                                   \
    node->name = am_lookup_param_name(L, 2);                            \
    return 1;                                                           \
}                                                                       \
static void get_read_mat##D##_node_value(lua_State *L, void *obj) {     \
    am_read_mat##D##_node *node = (am_read_mat##D##_node*)obj;          \
    am_mat##D *m = am_new_userdata(L, am_mat##D);                       \
    m->m = node->m;                                                     \
}                                                                       \
static am_property read_mat##D##_node_value_property =                  \
    {get_read_mat##D##_node_value, NULL};                               \
static void register_read_mat##D##_node_mt(lua_State *L) {              \
    lua_newtable(L);                                                    \
    lua_pushcclosure(L, am_scene_node_index, 0);                        \
    lua_setfield(L, -2, "__index");                                     \
    lua_pushcclosure(L, am_scene_node_newindex, 0);                     \
    lua_setfield(L, -2, "__newindex");                                  \
                                                                        \
    am_register_property(L, "value", &read_mat##D##_node_value_property); \
                                                                        \
    am_register_metatable(L, "read_mat" #D, MT_am_read_mat##D##_node, MT_am_scene_node);\
}

AM_READ_MAT_NODE_IMPL(2)
AM_READ_MAT_NODE_IMPL(3)
AM_READ_MAT_NODE_IMPL(4)

const char *am_program_param_type_name(am_program_param_type t) {
    switch (t) {
        case AM_PROGRAM_PARAM_UNIFORM_1F: return "float uniform";
        case AM_PROGRAM_PARAM_UNIFORM_2F: return "vec2 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_3F: return "vec3 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_4F: return "vec4 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_MAT2: return "mat2 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_MAT3: return "mat3 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_MAT4: return "mat4 uniform";
        case AM_PROGRAM_PARAM_UNIFORM_SAMPLER2D: return "sampler2D uniform";
        case AM_PROGRAM_PARAM_ATTRIBUTE_1F: return "float attribute array";
        case AM_PROGRAM_PARAM_ATTRIBUTE_2F: return "vec2 attribute array";
        case AM_PROGRAM_PARAM_ATTRIBUTE_3F: return "vec3 attribute array";
        case AM_PROGRAM_PARAM_ATTRIBUTE_4F: return "vec4 attribute array";
    }
    return NULL;
}

const char *am_program_param_client_type_name(am_program_param_name_slot *slot) {
    switch (slot->value.type) {
        case AM_PROGRAM_PARAM_CLIENT_TYPE_1F: return "float";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_2F: return "vec2";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_3F: return "vec3";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_4F: return "vec4";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT2: return "mat2";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT3: return "mat3";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_MAT4: return "mat4";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_ARRAY: {
            switch (slot->value.value.arr->type) {
                case AM_VIEW_TYPE_FLOAT: return "array of floats";
                case AM_VIEW_TYPE_FLOAT2: return "array of vec2s";
                case AM_VIEW_TYPE_FLOAT3: return "array of vec3s";
                case AM_VIEW_TYPE_FLOAT4: return "array of vec4s";
                case AM_VIEW_TYPE_UBYTE: return "array of ubytes";
                case AM_VIEW_TYPE_BYTE: return "array of bytes";
                case AM_VIEW_TYPE_UBYTE_NORM: return "array of normalized ubytes";
                case AM_VIEW_TYPE_BYTE_NORM: return "array of normalized bytes";
                case AM_VIEW_TYPE_USHORT: return "array of ushorts";
                case AM_VIEW_TYPE_SHORT: return "array of shorts";
                case AM_VIEW_TYPE_USHORT_ELEM: return "array of ushort indices";
                case AM_VIEW_TYPE_USHORT_NORM: return "array of normalized ushorts";
                case AM_VIEW_TYPE_SHORT_NORM: return "array of normalized shorts";
                case AM_VIEW_TYPE_UINT: return "array of uints";
                case AM_VIEW_TYPE_INT: return "array of ints";
                case AM_VIEW_TYPE_UINT_ELEM: return "array of uint indices";
                case AM_NUM_VIEW_TYPES: assert(false); break;
            }
            break;
        }
        case AM_PROGRAM_PARAM_CLIENT_TYPE_SAMPLER2D: return "texture2d";
        case AM_PROGRAM_PARAM_CLIENT_TYPE_UNDEFINED: return "uninitialized parameter";
    }
    return NULL;
}

void am_open_program_module(lua_State *L) {
    luaL_Reg funcs[] = {
        {"program", create_program},
        {NULL, NULL}
    };
    am_open_module(L, AMULET_LUA_MODULE_NAME, funcs);
    register_program_mt(L);
    register_program_node_mt(L);
    register_bind_node_mt(L);
    register_read_mat2_node_mt(L);
    register_read_mat3_node_mt(L);
    register_read_mat4_node_mt(L);
}
