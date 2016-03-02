/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/** @brief Lua RapidJSON wrapper implementation @file */

#include "rapidjson_impl.h"

#include <rapidjson/document.h>
#include <rapidjson/encodings.h>
#include <rapidjson/error/en.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/schema.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <set>

#include "luasandbox_output.h"
#include "sandbox_impl.h"
#include "stream_reader_impl.h"

namespace rj = rapidjson;

typedef struct heka_json
{
  rj::Document          *doc;
  rj::Value             *val;
  char                  *insitu;
  std::set<rj::Value *> *refs;
} heka_json;


typedef struct heka_schema
{
  rj::SchemaDocument *doc;
} heka_schema;


typedef struct heka_object_iter
{
  rj::Value::MemberIterator *it;
  rj::Value::MemberIterator *end;
} heka_object_iterator;


const char *mozsvc_heka_json       = "mozsvc.heka_json";
const char *mozsvc_heka_json_table = "heka_json";

static const char *mozsvc_heka_schema = "mozsvc.heka_schema";
static const char *mozsvc_heka_object_iter = "mozsvc.heka_object_iter";


static rj::Value* check_value(lua_State *lua)
{
  int n = lua_gettop(lua);
  luaL_argcheck(lua, n >= 1 && n <= 2, 0, "invalid number of arguments");
  heka_json *hj = static_cast<heka_json *>
      (luaL_checkudata(lua, 1, mozsvc_heka_json));
  rj::Value *v = static_cast<rj::Value *>(lua_touserdata(lua, 2));
  if (!v) {
    int t = lua_type(lua, 2);
    if (t == LUA_TNONE) {
      v = hj->doc ? hj->doc : hj->val;
    } else if (t == LUA_TNIL) {
      v = NULL;
    } else {
      luaL_checktype(lua, 2, LUA_TLIGHTUSERDATA);
    }
  } else if (hj->refs->find(v) == hj->refs->end()) {
    luaL_error(lua, "invalid value");
  }
  return v;
}


static int schema_gc(lua_State *lua)
{
  heka_schema *hs = static_cast<heka_schema *>
      (luaL_checkudata(lua, 1, mozsvc_heka_schema));
  delete(hs->doc);
  return 0;
}


static int iter_gc(lua_State *lua)
{
  heka_object_iter *hoi = static_cast<heka_object_iter *>(lua_touserdata(lua, 1));
  delete(hoi->it);
  delete(hoi->end);
  return 0;
}


static int hj_gc(lua_State *lua)
{
  heka_json *hj = static_cast<heka_json *>
      (luaL_checkudata(lua, 1, mozsvc_heka_json));
  delete(hj->refs);
  delete(hj->doc);
  delete(hj->val);
  free(hj->insitu);
  return 0;
}


static int hj_parse_schema(lua_State *lua)
{
  const char *json = luaL_checkstring(lua, 1);
  heka_schema *hs = static_cast<heka_schema *>(lua_newuserdata(lua, sizeof*hs));
  hs->doc = NULL;
  luaL_getmetatable(lua, mozsvc_heka_schema);
  lua_setmetatable(lua, -2);

  { // allows doc to be destroyed before the longjmp
    rj::Document doc;
    if (doc.Parse(json).HasParseError()) {
      lua_pushfstring(lua, "failed to parse offset:%f %s",
                      (lua_Number)doc.GetErrorOffset(),
                      rj::GetParseError_En(doc.GetParseError()));
    } else {
      hs->doc = new rj::SchemaDocument(doc);
      if (!hs->doc) {
        lua_pushstring(lua, "memory allocation failed");
      }
    }
  }
  if (!hs->doc) return lua_error(lua);
  return 1;
}


static int hj_parse(lua_State *lua)
{
  const char *json = luaL_checkstring(lua, 1);
  heka_json *hj = static_cast<heka_json *>(lua_newuserdata(lua, sizeof*hj));
  hj->doc = new rj::Document;
  hj->val = NULL;
  hj->insitu = NULL;
  hj->refs = new std::set<rj::Value *>;
  luaL_getmetatable(lua, mozsvc_heka_json);
  lua_setmetatable(lua, -2);

  if (!hj->doc || !hj->refs) {
    lua_pushstring(lua, "memory allocation failed");
    return lua_error(lua);
  } else {
    if (hj->doc->Parse(json).HasParseError()) {
      lua_pushfstring(lua, "failed to parse offset:%f %s",
                      (lua_Number)hj->doc->GetErrorOffset(),
                      rj::GetParseError_En(hj->doc->GetParseError()));
      return lua_error(lua);
    }
  }
  hj->refs->insert(hj->doc);
  return 1;
}


static int hj_validate(lua_State *lua)
{
  heka_json *hj = static_cast<heka_json *>
      (luaL_checkudata(lua, 1, mozsvc_heka_json));
  heka_schema *hs = static_cast<heka_schema *>
      (luaL_checkudata(lua, 2, mozsvc_heka_schema));

  rj::SchemaValidator validator(*hs->doc);
  rj::Value *v = hj->doc ? hj->doc : hj->val;
  if (!v->Accept(validator)) {
    lua_pushboolean(lua, false);
    luaL_Buffer b;
    luaL_buffinit(lua, &b);
    rj::StringBuffer sb;
    validator.GetInvalidSchemaPointer().StringifyUriFragment(sb);
    luaL_addstring(&b, "SchemaURI: ");
    luaL_addstring(&b, sb.GetString());
    luaL_addstring(&b, " Keyword: ");
    luaL_addstring(&b, validator.GetInvalidSchemaKeyword());
    sb.Clear();
    validator.GetInvalidDocumentPointer().StringifyUriFragment(sb);
    luaL_addstring(&b, " DocumentURI: ");
    luaL_addstring(&b, sb.GetString());
    luaL_pushresult(&b);
  }
  return 2; // ok, err
}


static int hj_find(lua_State *lua)
{
  heka_json *hj = static_cast<heka_json *>
      (luaL_checkudata(lua, 1, mozsvc_heka_json));

  int start = 3;
  rj::Value *v = static_cast<rj::Value *>(lua_touserdata(lua, 2));
  if (!v) {
    v = hj->doc ? hj->doc : hj->val;
    start = 2;
  } else if (hj->refs->find(v) == hj->refs->end()) {
    return luaL_error(lua, "invalid value");
  }

  int n = lua_gettop(lua);
  for (int i = start; i <= n; ++i) {
    switch (lua_type(lua, i)) {
    case LUA_TSTRING:
      {
        if (!v->IsObject()) {
          lua_pushnil(lua);
          return 1;
        }
        rj::Value::MemberIterator itr = v->FindMember(lua_tostring(lua, i));
        if (itr == v->MemberEnd()) {
          lua_pushnil(lua);
          return 1;
        }
        v = &itr->value;
      }
      break;
    case LUA_TNUMBER:
      {
        if (!v->IsArray()) {
          lua_pushnil(lua);
          return 1;
        }
        rj::SizeType idx = static_cast<rj::SizeType>(lua_tonumber(lua, i));
        if (idx >= v->Size()) {
          lua_pushnil(lua);
          return 1;
        }
        v = &(*v)[idx];
      }
      break;
    default:
      lua_pushnil(lua);
      return 1;
    }
  }
  hj->refs->insert(v);
  lua_pushlightuserdata(lua, v);
  return 1;
}


static int hj_type(lua_State *lua)
{
  rj::Value *v = check_value(lua);
  if (!v) {
    lua_pushnil(lua);
    return 1;
  }

  switch (v->GetType()) {
  case rj::kStringType:
    lua_pushstring(lua, "string");
    break;
  case rj::kNumberType:
    lua_pushstring(lua, "number");
    break;
  case rj::kFalseType:
  case rj::kTrueType:
    lua_pushstring(lua, "boolean");
    break;
  case rj::kObjectType:
    lua_pushstring(lua, "object");
    break;
  case rj::kArrayType:
    lua_pushstring(lua, "array");
    break;
  case rj::kNullType:
    lua_pushstring(lua, "null");
    break;
  }
  return 1;
}


static int hj_size(lua_State *lua)
{
  rj::Value *v = check_value(lua);
  if (!v) {
    lua_pushnil(lua);
    return 1;
  }

  switch (v->GetType()) {
  case rj::kStringType:
    lua_pushnumber(lua, (lua_Number)v->GetStringLength());
    break;
  case rj::kNumberType:
    return luaL_error(lua, "attempt to get length of a number");
  case rj::kFalseType:
  case rj::kTrueType:
    return luaL_error(lua, "attempt to get length of a boolean");
  case rj::kObjectType:
    lua_pushnumber(lua, (lua_Number)v->MemberCount());
    break;
  case rj::kArrayType:
    lua_pushnumber(lua, (lua_Number)v->Size());
    break;
  case rj::kNullType:
    return luaL_error(lua, "attempt to get length of a NULL");
  }
  return 1;
}


static int hj_make_field(lua_State *lua)
{
  rj::Value *v = check_value(lua);
  if (!v) {
    lua_pushnil(lua);
    return 1;
  }

  lua_createtable(lua, 0, 2);
  lua_pushlightuserdata(lua, (void *)v);
  lua_setfield(lua, -2, "value");
  lua_pushvalue(lua, 1);
  lua_setfield(lua, -2, "userdata");
  return 1;
}


static int hj_object_iter(lua_State *lua)
{
  heka_object_iter *hoi = static_cast<heka_object_iter *>
      (lua_touserdata(lua, lua_upvalueindex(1)));
  rj::Value *v = (rj::Value *)lua_touserdata(lua, lua_upvalueindex(2));
  heka_json *hj = (heka_json *)lua_touserdata(lua, lua_upvalueindex(3));

  if (hj->refs->find(v) == hj->refs->end()) {
    return luaL_error(lua, "iterator has been invalidated");
  }

  if (*hoi->it != *hoi->end) {
    rj::Value *next = &(*hoi->it)->value;
    hj->refs->insert(next);
    lua_pushlstring(lua, (*hoi->it)->name.GetString(),
                    (size_t)(*hoi->it)->name.GetStringLength());
    lua_pushlightuserdata(lua, next);
    ++*hoi->it;
  } else {
    lua_pushnil(lua);
    lua_pushnil(lua);
  }
  return 2;
}


static int hj_array_iter(lua_State *lua)
{
  rj::SizeType it = (rj::SizeType)lua_tonumber(lua, lua_upvalueindex(1));
  rj::SizeType end = (rj::SizeType)lua_tonumber(lua, lua_upvalueindex(2));
  rj::Value *v = (rj::Value *)lua_touserdata(lua, lua_upvalueindex(3));
  heka_json *hj = (heka_json *)lua_touserdata(lua, lua_upvalueindex(4));

  if (hj->refs->find(v) == hj->refs->end()) {
    return luaL_error(lua, "iterator has been invalidated");
  }

  if (it != end) {
    rj::Value *next = &(*v)[it];
    hj->refs->insert(next);
    lua_pushnumber(lua, (lua_Number)it);
    lua_pushlightuserdata(lua, next);

    ++it;
    lua_pushnumber(lua, (lua_Number)it);
    lua_replace(lua, lua_upvalueindex(1));
  } else {
    lua_pushnil(lua);
    lua_pushnil(lua);
  }
  return 2;
}


static int hj_value(lua_State *lua)
{
  rj::Value *v = check_value(lua);
  if (!v) {
    lua_pushnil(lua);
    return 1;
  }

  switch (v->GetType()) {
  case rj::kStringType:
    lua_pushlstring(lua, v->GetString(), (size_t)v->GetStringLength());
    break;
  case rj::kNumberType:
    lua_pushnumber(lua, (lua_Number)v->GetDouble());
    break;
  case rj::kFalseType:
  case rj::kTrueType:
    lua_pushboolean(lua, v->GetBool());
    break;
  case rj::kObjectType:
    return luaL_error(lua, "value() not allowed on an object");
    break;
  case rj::kArrayType:
    return luaL_error(lua, "value() not allowed on an array");
    break;
  default:
    lua_pushnil(lua);
    break;
  }
  return 1;
}


static int hj_iter(lua_State *lua)
{
  rj::Value *v = check_value(lua);
  if (!v) {
    lua_pushnil(lua);
    return 1;
  }

  switch (v->GetType()) {
  case rj::kObjectType:
    {
      heka_object_iter *hoi = static_cast<heka_object_iter *>
          (lua_newuserdata(lua, sizeof*hoi));
      hoi->it = new rj::Value::MemberIterator;
      hoi->end = new rj::Value::MemberIterator;
      luaL_getmetatable(lua, mozsvc_heka_object_iter);
      lua_setmetatable(lua, -2);
      if (!hoi->it || !hoi->end) {
        return luaL_error(lua, "memary allocation failure");
      }
      *hoi->it = v->MemberBegin();
      *hoi->end = v->MemberEnd();
      lua_pushlightuserdata(lua, (void *)v);
      lua_pushvalue(lua, 1);
      lua_pushcclosure(lua, hj_object_iter, 3);
    }
    break;
  case rj::kArrayType:
    {
      lua_pushnumber(lua, 0);
      lua_pushnumber(lua, (lua_Number)v->Size());
      lua_pushlightuserdata(lua, (void *)v);
      lua_pushvalue(lua, 1);
      lua_pushcclosure(lua, hj_array_iter, 4);
    }
    break;
  default:
    return luaL_error(lua, "iter() not allowed on a primitive type");
    break;
  }
  return 1;
}


lsb_const_string read_message(lua_State *lua, int idx, lsb_heka_message *m)
{
  lsb_const_string ret = { NULL, 0 };
  size_t field_len;
  const char *field = luaL_checklstring(lua, idx, &field_len);
  int fi = (int)luaL_optinteger(lua, idx + 1, 0);
  luaL_argcheck(lua, fi >= 0, idx + 1, "field index must be >= 0");
  int ai = (int)luaL_optinteger(lua, idx + 2, 0);
  luaL_argcheck(lua, ai >= 0, idx + 2, "array index must be >= 0");

  if (strcmp(field, LSB_PAYLOAD) == 0) {
    if (m->payload.s) ret = m->payload;
  } else {
    if (field_len >= 8
        && memcmp(field, LSB_FIELDS "[", 7) == 0
        && field[field_len - 1] == ']') {
      lsb_read_value v;
      lsb_const_string f = { field + 7, field_len - 8 };
      lsb_read_heka_field(m, &f, fi, ai, &v);
      if (v.type == LSB_READ_STRING) ret = v.u.s;
    }
  }
  return ret;
}


static int hj_parse_message(lua_State *lua)
{
  lsb_lua_sandbox *lsb = static_cast<lsb_lua_sandbox *>
      (lua_touserdata(lua, lua_upvalueindex(1)));
  if (!lsb) {
    return luaL_error(lua, "%s() invalid lightuserdata", __func__);
  }
  int n = lua_gettop(lua);
  int idx = 1;

  lsb_heka_message *msg = NULL;
  lsb_heka_sandbox *hsb = static_cast<lsb_heka_sandbox *>(lsb_get_parent(lsb));
  if (hsb->type == 'i') {
    luaL_argcheck(lua, n >= 2 && n <= 4, 0, "invalid number of arguments");
    heka_stream_reader *hsr = static_cast<heka_stream_reader *>
        (luaL_checkudata(lua, 1, mozsvc_heka_stream_reader));
    msg = &hsr->msg;
    idx = 2;
  } else {
    luaL_argcheck(lua, n >= 1 && n <= 3, 0, "invalid number of arguments");
    if (!hsb->msg || !hsb->msg->raw.s) {
      return luaL_error(lua, "no active message");
    }
    msg = hsb->msg;
  }

  lsb_const_string json = read_message(lua, idx, msg);
  if (!json.s) return luaL_error(lua, "field not found");

  char *inflated = NULL;
#ifdef HAVE_ZLIB
  // automatically handle gzipped strings (optimization for Mozilla telemetry
  // messages)
  if (json.len > 2) {
    if (json.s[0] == 0x1f && (unsigned char)json.s[1] == 0x8b) {
      size_t mms = (size_t)lua_tointeger(lua, lua_upvalueindex(2));
      inflated = lsb_ungzip(json.s, json.len, mms, NULL);
      if (!inflated) return luaL_error(lua, "lsb_ungzip failed");
    }
  }
#endif

  heka_json *hj = static_cast<heka_json *>(lua_newuserdata(lua, sizeof*hj));
  hj->doc = new rj::Document;
  hj->val = NULL;
  hj->insitu = inflated;
  hj->refs =  new std::set<rj::Value *>;
  luaL_getmetatable(lua, mozsvc_heka_json);
  lua_setmetatable(lua, -2);

  if (!hj->doc || !hj->refs) {
    lua_pushstring(lua, "memory allocation failed");
    return lua_error(lua);
  }

  bool err = false;
  if (hj->insitu) {
    if (hj->doc->ParseInsitu<rj::kParseStopWhenDoneFlag>(hj->insitu)
        .HasParseError()) {
      err = true;
      lua_pushfstring(lua, "failed to parse offset:%f %s",
                      (lua_Number)hj->doc->GetErrorOffset(),
                      rj::GetParseError_En(hj->doc->GetParseError()));
    }
  } else {
    rj::MemoryStream ms(json.s, json.len);
    if (hj->doc->ParseStream<0, rj::UTF8<> >(ms).HasParseError()) {
      err = true;
      lua_pushfstring(lua, "failed to parse offset:%f %s",
                      (lua_Number)hj->doc->GetErrorOffset(),
                      rj::GetParseError_En(hj->doc->GetParseError()));
    }
  }

  if (err) return lua_error(lua);
  hj->refs->insert(hj->doc);
  return 1;
}


class OutputBufferWrapper {
public:
  typedef char Ch;
  OutputBufferWrapper(lsb_output_buffer *ob) : ob_(ob), err_(NULL) { }
#if _BullseyeCoverage
#pragma BullseyeCoverage off
#endif
  Ch Peek() const { assert(false); return '\0'; }
  Ch Take() { assert(false); return '\0'; }
  size_t Tell() const { return 0; }
  Ch* PutBegin() { assert(false); return 0; }
  size_t PutEnd(Ch *) { assert(false); return 0; }
#if _BullseyeCoverage
#pragma BullseyeCoverage on
#endif
  void Put(Ch c)
  {
    const char *err = lsb_outputc(ob_, c);
    if (err) err_ = err;
  }
  void Flush() { return; }
  const char* GetError() { return err_; }
private:
  OutputBufferWrapper(const OutputBufferWrapper&);
  OutputBufferWrapper& operator=(const OutputBufferWrapper&);
  lsb_output_buffer *ob_;
  const char *err_;
};


static int output_heka_json(lua_State *lua)
{
  lsb_output_buffer *ob = static_cast<lsb_output_buffer *>
      (lua_touserdata(lua, -1));
  heka_json *hj = static_cast<heka_json *>(lua_touserdata(lua, -2));
  rj::Value *v = static_cast<rj::Value *>(lua_touserdata(lua, -3));
  if (!(ob && hj)) {
    return 1;
  }
  if (!v) {
    v = hj->doc ? hj->doc : hj->val;
  } else {
    if (hj->refs->find(v) == hj->refs->end()) {
      return 1;
    }
  }
  OutputBufferWrapper obw(ob);
  rapidjson::Writer<OutputBufferWrapper> writer(obw);
  v->Accept(writer);
  return obw.GetError() == NULL ? 0 : 1;
}


static int hj_remove(lua_State *lua)
{
  hj_find(lua);
  rj::Value *v = static_cast<rj::Value *>(lua_touserdata(lua, -1));
  if (!v) {
    lua_pushnil(lua);
    return 1;
  }

  heka_json *hj = static_cast<heka_json *>
      (luaL_checkudata(lua, 1, mozsvc_heka_json));

  heka_json *nv = static_cast<heka_json *>(lua_newuserdata(lua, sizeof*nv));
  nv->doc = NULL;
  nv->val = new rj::Value;
  nv->insitu = NULL;
  nv->refs = new std::set<rj::Value *>;
  luaL_getmetatable(lua, mozsvc_heka_json);
  lua_setmetatable(lua, -2);

  if (!nv->val || !nv->refs) {
    lua_pushstring(lua, "memory allocation failed");
    return lua_error(lua);
  }

  *nv->val = *v; // move the value out replacing the original with NULL
  hj->refs->erase(v);
  nv->refs->insert(nv->val);
  return 1;
}


static const struct luaL_reg schemalib_m[] =
{
  { "__gc", schema_gc },
  { NULL, NULL }
};


static const struct luaL_reg iterlib_m[] =
{
  { "__gc", iter_gc },
  { NULL, NULL }
};


static const struct luaL_reg hjlib_f[] =
{
  { "parse_schema", hj_parse_schema },
  { "parse", hj_parse },
  { NULL, NULL }
};


static const struct luaL_reg hjlib_m[] =
{
  { "validate", hj_validate },
  { "type", hj_type },
  { "find", hj_find },
  { "value", hj_value },
  { "iter", hj_iter },
  { "size", hj_size },
  { "make_field", hj_make_field },
  { "remove", hj_remove },
  { "__gc", hj_gc },
  { NULL, NULL }
};


int luaopen_heka_json(lua_State *lua)
{
  lua_newtable(lua);
  lsb_add_output_function(lua, output_heka_json);
  lua_replace(lua, LUA_ENVIRONINDEX);

  luaL_newmetatable(lua, mozsvc_heka_schema);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, schemalib_m);
  lua_pop(lua, 1);

  luaL_newmetatable(lua, mozsvc_heka_object_iter);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, iterlib_m);
  lua_pop(lua, 1);

  luaL_newmetatable(lua, mozsvc_heka_json);
  lua_pushvalue(lua, -1);
  lua_setfield(lua, -2, "__index");
  luaL_register(lua, NULL, hjlib_m);
  luaL_register(lua, mozsvc_heka_json_table, hjlib_f);
  // special case parse_message since it needs access to the sandbox
  lua_getfield(lua, LUA_REGISTRYINDEX, LSB_CONFIG_TABLE);
  if (lua_type(lua, -1) != LUA_TTABLE) {
    return luaL_error(lua, LSB_CONFIG_TABLE " is missing");
  }
  lsb_lua_sandbox *lsb = static_cast<lsb_lua_sandbox *>
      (lua_touserdata(lua, lua_upvalueindex(1)));
  lua_pushlightuserdata(lua, static_cast<void *>(lsb));
  lua_getfield(lua, -2, LSB_HEKA_MAX_MESSAGE_SIZE);
  lua_pushcclosure(lua, hj_parse_message, 2);
  lua_setfield(lua, -3, "parse_message");
  lua_pop(lua, 1); // remove LSB_CONFIG_TABLE
  return 1;
}
