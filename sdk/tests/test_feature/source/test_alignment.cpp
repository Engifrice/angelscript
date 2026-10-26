#include "utils.h"
#include <stdint.h>

namespace TestAlignment
{

// Counts allocations made through the object memory functions, and verifies that the
// engine always passes a valid alignment and gives back the same pointers
struct AllocatorState
{
	int    allocs;
	int    frees;
	int    invalidRequests;
	size_t maxAlignmentRequested;
};

static void *TestAlignedAlloc(size_t size, size_t alignment, void *userParam)
{
	AllocatorState *state = (AllocatorState*)userParam;
	if( alignment == 0 || (alignment & (alignment - 1)) != 0 )
		state->invalidRequests++;
	if( alignment > state->maxAlignmentRequested )
		state->maxAlignmentRequested = alignment;
	state->allocs++;

	// Keep the original pointer just before the aligned pointer
	char *mem = (char*)malloc(size + alignment + sizeof(void*));
	char *amem = mem + sizeof(void*);
	if( (uintptr_t)amem & (alignment - 1) )
		amem += alignment - ((uintptr_t)amem & (alignment - 1));
	((void**)amem)[-1] = mem;
	return amem;
}

static void TestAlignedFree(void *ptr, void *userParam)
{
	AllocatorState *state = (AllocatorState*)userParam;
	state->frees++;
	free(((void**)ptr)[-1]);
}

// A non-pod value type that requires 16 byte alignment and checks it on construction
struct alignas(16) Vec16
{
	float v[8];
};

static int g_misalignedVec16 = 0;

static void Vec16_Construct(asIScriptGeneric *gen)
{
	void *self = gen->GetObject();
	if( (uintptr_t)self % 16 )
		g_misalignedVec16++;
	new(self) Vec16();
}

static void Vec16_Destruct(asIScriptGeneric *gen)
{
	((Vec16*)gen->GetObject())->~Vec16();
}

static void Vec16_Assign(asIScriptGeneric *gen)
{
	Vec16 *self = (Vec16*)gen->GetObject();
	*self = *(Vec16*)gen->GetArgObject(0);
	gen->SetReturnAddress(self);
}

static int RegisterVec16(asIScriptEngine *engine)
{
	int r = engine->RegisterObjectType("vec16", (int)sizeof(Vec16), (int)alignof(Vec16), asOBJ_VALUE | asOBJ_APP_CLASS_CD); if( r < 0 ) return r;
	r = engine->RegisterObjectBehaviour("vec16", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Vec16_Construct), asCALL_GENERIC); if( r < 0 ) return r;
	r = engine->RegisterObjectBehaviour("vec16", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Vec16_Destruct), asCALL_GENERIC); if( r < 0 ) return r;
	r = engine->RegisterObjectMethod("vec16", "vec16 &opAssign(const vec16 &in)", asFUNCTION(Vec16_Assign), asCALL_GENERIC); if( r < 0 ) return r;
	return 0;
}

// Script using vec16 only in places that are allocated on the heap. Locals on the
// script stack are not aligned yet, so they are not tested here
static const char *heapScript =
	"class Holder        \n"
	"{                   \n"
	"  vec16 a;          \n"
	"  vec16 b;          \n"
	"}                   \n"
	"void main()         \n"
	"{                   \n"
	"  Holder @h = Holder(); \n"
	"  Holder @h2 = Holder(); \n"
	"}                   \n";

// Returns the alignment of the type, or 0 if the type doesn't exist
static asUINT AlignmentOf(asITypeInfo *type, const char *name)
{
	if( type == 0 )
	{
		PRINTF("Type '%s' not found\n", name);
		return 0;
	}
	return type->GetAlignment();
}

bool Test()
{
	bool fail = false;
	int r;
	CBufferedOutStream bout;
	asIScriptEngine *engine;

	// Validation of the alignment informed at registration
	{
		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		// Not a power of two
		r = engine->RegisterObjectType("a3", 12, 3, asOBJ_VALUE | asOBJ_POD);
		if( r != asINVALID_ARG )
			TEST_FAILED;
		if( bout.buffer.find("The alignment 3 must be a power of two, and not larger than 16") == std::string::npos )
			TEST_FAILED;

		// Zero
		bout.buffer = "";
		r = engine->RegisterObjectType("a0", 12, 0, asOBJ_VALUE | asOBJ_POD);
		if( r != asINVALID_ARG )
			TEST_FAILED;

		// Larger than the maximum supported alignment
		bout.buffer = "";
		r = engine->RegisterObjectType("a32", 32, 32, asOBJ_VALUE | asOBJ_POD);
		if( r != asINVALID_ARG )
			TEST_FAILED;
		if( bout.buffer.find("The alignment 32 must be a power of two, and not larger than 16") == std::string::npos )
			TEST_FAILED;

		// The size must be a multiple of the alignment
		bout.buffer = "";
		r = engine->RegisterObjectType("a12", 12, 8, asOBJ_VALUE | asOBJ_POD);
		if( r != asINVALID_ARG )
			TEST_FAILED;
		if( bout.buffer.find("The size 12 must be a multiple of the alignment 8") == std::string::npos )
			TEST_FAILED;

		// Valid registrations keep the informed alignment
		bout.buffer = "";
		r = engine->RegisterObjectType("a16", 32, 16, asOBJ_VALUE | asOBJ_POD);
		if( r < 0 )
			TEST_FAILED;
		r = engine->RegisterObjectType("a1", 3, 1, asOBJ_VALUE | asOBJ_POD);
		if( r < 0 )
			TEST_FAILED;
		if( AlignmentOf(engine->GetTypeInfoByName("a16"), "a16") != 16 )
			TEST_FAILED;
		if( AlignmentOf(engine->GetTypeInfoByName("a1"), "a1") != 1 )
			TEST_FAILED;
		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Alignment derived from the size when it is not informed
	{
		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		struct { const char *name; int size; asUINT expected; } cases[] =
		{
			{"s1", 1, 1}, {"s2", 2, 2}, {"s3", 3, 1}, {"s6", 6, 2}, {"s12", 12, 4},
			{"s16", 16, 8}, {"s24", 24, 8}, {"s32", 32, 8}, {"s48", 48, 8}
		};
		for( asUINT n = 0; n < sizeof(cases)/sizeof(cases[0]); n++ )
		{
			r = engine->RegisterObjectType(cases[n].name, cases[n].size, asOBJ_VALUE | asOBJ_POD);
			if( r < 0 )
				TEST_FAILED;
			else if( AlignmentOf(engine->GetTypeInfoByName(cases[n].name), cases[n].name) != cases[n].expected )
			{
				PRINTF("Type of size %d got alignment %d, expected %d\n", cases[n].size, AlignmentOf(engine->GetTypeInfoByName(cases[n].name), cases[n].name), cases[n].expected);
				TEST_FAILED;
			}
		}

		// Reference types without a size get pointer alignment
		r = engine->RegisterObjectType("ref", 0, asOBJ_REF | asOBJ_NOCOUNT);
		if( r < 0 )
			TEST_FAILED;
		else if( AlignmentOf(engine->GetTypeInfoByName("ref"), "ref") != sizeof(void*) )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Alignment of enums, typedefs, funcdefs, template instances and script classes
	{
		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		engine->RegisterEnum("E32");
		engine->RegisterEnum("E8", "int8");
		engine->RegisterEnum("E64", "uint64");
		engine->RegisterTypedef("real", "double");
		engine->RegisterFuncdef("void CB()");
		RegisterScriptArray(engine, false);

		if( AlignmentOf(engine->GetTypeInfoByName("E32"), "E32") != 4 )
			TEST_FAILED;
		if( AlignmentOf(engine->GetTypeInfoByName("E8"), "E8") != 1 )
			TEST_FAILED;
		if( AlignmentOf(engine->GetTypeInfoByName("E64"), "E64") != 8 )
			TEST_FAILED;
		if( AlignmentOf(engine->GetTypeInfoByName("real"), "real") != 8 )
			TEST_FAILED;
		if( AlignmentOf(engine->GetTypeInfoByDecl("CB"), "CB") != sizeof(void*) )
			TEST_FAILED;

		// The template instance takes the alignment of the template
		asITypeInfo *arrType = engine->GetTypeInfoByDecl("array<int>");
		if( arrType == 0 || arrType->GetAlignment() != AlignmentOf(engine->GetTypeInfoByName("array"), "array") )
			TEST_FAILED;

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"enum SE16 : int16 { A } \n"
			"enum SE64 : int64 { B } \n"
			"funcdef void SCB(); \n"
			"class C { int a; } \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		else
		{
			if( AlignmentOf(mod->GetTypeInfoByName("SE16"), "SE16") != 2 )
				TEST_FAILED;
			if( AlignmentOf(mod->GetTypeInfoByName("SE64"), "SE64") != 8 )
				TEST_FAILED;
			if( AlignmentOf(mod->GetTypeInfoByName("SCB"), "SCB") != sizeof(void*) )
				TEST_FAILED;
			if( AlignmentOf(mod->GetTypeInfoByName("C"), "C") < sizeof(void*) )
				TEST_FAILED;
		}

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Heap allocated instances are aligned with the default object memory functions
	{
		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		r = RegisterVec16(engine);
		if( r < 0 )
			TEST_FAILED;

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", heapScript);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		g_misalignedVec16 = 0;
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		void *obj = engine->CreateScriptObject(engine->GetTypeInfoByName("vec16"));
		if( obj == 0 || (uintptr_t)obj % 16 )
			TEST_FAILED;
		engine->ReleaseScriptObject(obj, engine->GetTypeInfoByName("vec16"));

		if( g_misalignedVec16 != 0 )
		{
			PRINTF("%d vec16 instances were not aligned\n", g_misalignedVec16);
			TEST_FAILED;
		}

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Object memory goes through the application's memory functions
	{
		AllocatorState state = {};

		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		// Both functions must be informed
		r = engine->SetObjectMemoryFunctions(TestAlignedAlloc, 0, &state);
		if( r != asINVALID_ARG )
			TEST_FAILED;
		bout.buffer = "";

		r = engine->SetObjectMemoryFunctions(TestAlignedAlloc, TestAlignedFree, &state);
		if( r < 0 )
			TEST_FAILED;

		r = RegisterVec16(engine);
		if( r < 0 )
			TEST_FAILED;

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", heapScript);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		g_misalignedVec16 = 0;
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;

		void *obj = engine->CreateScriptObject(engine->GetTypeInfoByName("vec16"));
		if( obj == 0 || (uintptr_t)obj % 16 )
			TEST_FAILED;
		engine->ReleaseScriptObject(obj, engine->GetTypeInfoByName("vec16"));

		engine->ShutDownAndRelease();

		// Two script objects, each with two vec16 members, plus the object created by the application
		if( state.allocs < 7 )
		{
			PRINTF("Expected at least 7 allocations, got %d\n", state.allocs);
			TEST_FAILED;
		}
		// Script objects are freed through the same functions, so everything must be released
		if( state.allocs != state.frees )
		{
			PRINTF("%d allocations but %d frees\n", state.allocs, state.frees);
			TEST_FAILED;
		}
		if( state.invalidRequests != 0 )
			TEST_FAILED;
		if( state.maxAlignmentRequested != 16 )
		{
			PRINTF("The largest alignment requested was %d, expected 16\n", (int)state.maxAlignmentRequested);
			TEST_FAILED;
		}
		if( g_misalignedVec16 != 0 )
		{
			PRINTF("%d vec16 instances were not aligned\n", g_misalignedVec16);
			TEST_FAILED;
		}

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}
	}

	return fail;
}

} // namespace TestAlignment
