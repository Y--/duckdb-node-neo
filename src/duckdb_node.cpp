#define NODE_API_NO_EXTERNAL_BUFFERS_ALLOWED // apparently electron does not like those

#include "napi.h"

// this file contains generated template instantiations from duckdb.h
#include "duckdb_node_generated.cpp"

// dependent on the runtime we may or may not be allowed to pass pointers back and forth
// see here: https://github.com/nodejs/node-addon-api/blob/main/doc/external_buffer.md
static Napi::Value CopyBuffer(const Napi::CallbackInfo &info) {
	Napi::Env env = info.Env();
	auto pointer = duckdb_node::ValueConversion::FromJS<void *>(info, 0);
	if (!pointer) {
		return env.Null();
	}
	auto n = duckdb_node::ValueConversion::FromJS<idx_t>(info, 1);
	return Napi::Buffer<uint8_t>::NewOrCopy(env, (uint8_t *)pointer, n);
}

// special case handling for api functions that take a char**
static Napi::Value OutGetString(const Napi::CallbackInfo &info) {
	Napi::Env env = info.Env();
	auto pp = duckdb_node::ValueConversion::FromJS<const char **>(info, 0);
	return duckdb_node::ValueConversion::ToJS(env, *pp);
}

// strings are the only types with pointers in them
static Napi::Value ConvertStringVector(const Napi::CallbackInfo &info) {
	Napi::Env env = info.Env();
	auto vector = duckdb_node::ValueConversion::FromJS<duckdb_vector>(info, 0);
	auto n = duckdb_node::ValueConversion::FromJS<idx_t>(info, 1);

	auto type_id = duckdb_get_type_id(duckdb_vector_get_column_type(vector));
	if (type_id != DUCKDB_TYPE_BLOB && type_id != DUCKDB_TYPE_VARCHAR) {
		return env.Null();
	}
	auto array = Napi::Array::New(env, n);
	auto strings = (duckdb_string_t *)duckdb_vector_get_data(vector);
	auto validity = duckdb_vector_get_validity(vector);

	for (idx_t row_idx = 0; row_idx < n; row_idx++) {
		if (!duckdb_validity_row_is_valid(validity, row_idx)) {
			array[row_idx] = env.Null();
			continue;
		}
		if (duckdb_string_is_inlined(strings[row_idx])) {
			array[row_idx] = Napi::Buffer<uint8_t>::NewOrCopy(env, (uint8_t *)strings[row_idx].value.inlined.inlined,
			                                                  strings[row_idx].value.inlined.length);
		} else {
			array[row_idx] = Napi::Buffer<uint8_t>::NewOrCopy(env, (uint8_t *)strings[row_idx].value.pointer.ptr,
			                                                  strings[row_idx].value.pointer.length);
		}
	}
	return array;
}

class DuckDBNodeNative : public Napi::Addon<DuckDBNodeNative> {
public:
	DuckDBNodeNative(Napi::Env env, Napi::Object exports) {
		RegisterGenerated(env, exports);

		exports.Set(Napi::String::New(env, "copy_buffer"), Napi::Function::New<CopyBuffer>(env));
		exports.Set(
		    Napi::String::New(env, "out_string_wrapper"),
		    duckdb_node::PointerHolder<duckdb_node::out_string_wrapper>::Init(env, "out_string_wrapper")->Value());
		exports.Set(Napi::String::New(env, "out_get_string"), Napi::Function::New<OutGetString>(env));
		exports.Set(Napi::String::New(env, "convert_string_vector"), Napi::Function::New<ConvertStringVector>(env));
	}
};

NODE_API_ADDON(DuckDBNodeNative);
