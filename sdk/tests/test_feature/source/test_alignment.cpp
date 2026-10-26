#include "utils.h"
#include "../../../add_on/scriptdictionary/scriptdictionary.h"
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
static int g_aliveVec16 = 0;

static void Vec16_Construct(asIScriptGeneric *gen)
{
	void *self = gen->GetObject();
	if( (uintptr_t)self % 16 )
		g_misalignedVec16++;
	new(self) Vec16();
	g_aliveVec16++;
}

static void Vec16_CopyConstruct(asIScriptGeneric *gen)
{
	void *self = gen->GetObject();
	if( (uintptr_t)self % 16 )
		g_misalignedVec16++;
	new(self) Vec16(*(Vec16*)gen->GetArgObject(0));
	g_aliveVec16++;
}

static void Vec16_Destruct(asIScriptGeneric *gen)
{
	((Vec16*)gen->GetObject())->~Vec16();
	g_aliveVec16--;
}

static void Vec16_Assign(asIScriptGeneric *gen)
{
	Vec16 *self = (Vec16*)gen->GetObject();
	*self = *(Vec16*)gen->GetArgObject(0);
	gen->SetReturnAddress(self);
}

// A pod value type that requires 16 byte alignment, so it is stored inline in script classes
struct alignas(16) Pod16
{
	float v[4];
};

// Returns the offset of the named property in the type, or -1 if not found
static int PropertyOffset(asITypeInfo *type, const char *name)
{
	for( asUINT n = 0; type && n < type->GetPropertyCount(); n++ )
	{
		const char *propName;
		int offset;
		type->GetProperty(n, &propName, 0, 0, 0, &offset);
		if( strcmp(propName, name) == 0 )
			return offset;
	}
	return -1;
}

static int RegisterVec16(asIScriptEngine *engine)
{
	int r = engine->RegisterObjectType("vec16", (int)sizeof(Vec16), (int)alignof(Vec16), asOBJ_VALUE | asOBJ_APP_CLASS_CD); if( r < 0 ) return r;
	r = engine->RegisterObjectBehaviour("vec16", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Vec16_Construct), asCALL_GENERIC); if( r < 0 ) return r;
	r = engine->RegisterObjectBehaviour("vec16", asBEHAVE_CONSTRUCT, "void f(const vec16 &in)", asFUNCTION(Vec16_CopyConstruct), asCALL_GENERIC); if( r < 0 ) return r;
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

// Counts arguments that are not aligned or have unexpected values when received by generic functions
static int g_badArgs = 0;

// void checkArgs(int8, double, int, int64, float, const pod16 &in, const vec16 &in)
static void CheckArgs_Generic(asIScriptGeneric *gen)
{
	// All values on the stack are aligned to 8 bytes
	for( int n = 0; n < gen->GetArgCount(); n++ )
		if( (uintptr_t)gen->GetAddressOfArg(n) % 8 )
			g_badArgs++;

	if( gen->GetArgByte(0) != 1 ) g_badArgs++;
	if( gen->GetArgDouble(1) != 2.5 ) g_badArgs++;
	if( gen->GetArgDWord(2) != 3 ) g_badArgs++;
	if( gen->GetArgQWord(3) != 4 ) g_badArgs++;
	if( gen->GetArgFloat(4) != 5.5f ) g_badArgs++;

	// The objects referred to are aligned to their own alignment
	if( (uintptr_t)gen->GetArgAddress(5) % 16 ) g_badArgs++;
	if( (uintptr_t)gen->GetArgAddress(6) % 16 ) g_badArgs++;
}

// void checkVarArg(const ?&in), the reference and the type id each take up a slot
static void CheckVarArg_Generic(asIScriptGeneric *gen)
{
	if( (uintptr_t)gen->GetAddressOfArg(0) % 8 )
		g_badArgs++;
	if( gen->GetArgTypeId(0) != asTYPEID_DOUBLE || *(double*)gen->GetArgAddress(0) != 2.5 )
		g_badArgs++;
}

// void checkPod(const pod16 &in)
static void CheckPod_Generic(asIScriptGeneric *gen)
{
	if( (uintptr_t)gen->GetArgAddress(0) % 16 )
		g_badArgs++;
}

// void checkDouble(double &out), the referred variable is a local variable aligned to 8 bytes
static void CheckDouble_Generic(asIScriptGeneric *gen)
{
	double *d = (double*)gen->GetArgAddress(0);
	if( (uintptr_t)d % 8 )
		g_badArgs++;
	*d = 1.5;
}

// vec16 genVec(), constructs the returned value directly in the return location
static void GenVec_Generic(asIScriptGeneric *gen)
{
	void *mem = gen->GetAddressOfReturnLocation();
	if( (uintptr_t)mem % 16 )
		g_misalignedVec16++;
	new(mem) Vec16();
	g_aliveVec16++;
}

// A value type whose size and alignment depend on how it is registered, to simulate a type that
// differs between platforms. Guard values are written at the start and end of the object, so
// overlapping variables would be detected.
static asUINT g_valSize = 0;
static asUINT g_valAlignment = 0;
static int g_badVals = 0;

static void CheckVal(const void *ptr)
{
	const asBYTE *p = (const asBYTE*)ptr;
	if( (uintptr_t)p % g_valAlignment ||
		*(const asUINT*)p != 0x12345678 ||
		*(const asUINT*)(p + g_valSize - 4) != 0x9ABCDEF0 )
		g_badVals++;
}

static void Val_Construct(asIScriptGeneric *gen)
{
	asBYTE *p = (asBYTE*)gen->GetObject();
	memset(p, 0, g_valSize);
	*(asUINT*)p = 0x12345678;
	*(asUINT*)(p + g_valSize - 4) = 0x9ABCDEF0;
	CheckVal(p);
}

static void Val_CopyConstruct(asIScriptGeneric *gen)
{
	CheckVal(gen->GetArgObject(0));
	memcpy(gen->GetObject(), gen->GetArgObject(0), g_valSize);
	CheckVal(gen->GetObject());
}

static void Val_Destruct(asIScriptGeneric *gen)
{
	CheckVal(gen->GetObject());
}

static void Val_Assign(asIScriptGeneric *gen)
{
	CheckVal(gen->GetObject());
	CheckVal(gen->GetArgObject(0));
	memcpy(gen->GetObject(), gen->GetArgObject(0), g_valSize);
	gen->SetReturnAddress(gen->GetObject());
}

static void Val_Check(asIScriptGeneric *gen)
{
	CheckVal(gen->GetArgAddress(0));
}

static void PodX_Check(asIScriptGeneric *gen)
{
	// The pod has the same alignment as the val type
	if( (uintptr_t)gen->GetArgAddress(0) % g_valAlignment )
		g_badVals++;
}

// Registers the types with the given size and alignment, as different platforms could do
static void RegisterPlatformTypes(asIScriptEngine *engine, asUINT valSize, asUINT valAlignment, asUINT podSize)
{
	engine->RegisterObjectType("val", (int)valSize, (int)valAlignment, asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectBehaviour("val", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(Val_Construct), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("val", asBEHAVE_CONSTRUCT, "void f(const val &in)", asFUNCTION(Val_CopyConstruct), asCALL_GENERIC);
	engine->RegisterObjectBehaviour("val", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(Val_Destruct), asCALL_GENERIC);
	engine->RegisterObjectMethod("val", "val &opAssign(const val &in)", asFUNCTION(Val_Assign), asCALL_GENERIC);
	engine->RegisterObjectType("podx", (int)podSize, (int)valAlignment, asOBJ_VALUE | asOBJ_POD);
	engine->RegisterGlobalFunction("void checkVal(const val &in)", asFUNCTION(Val_Check), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void checkPodx(const podx &in)", asFUNCTION(PodX_Check), asCALL_GENERIC);
	engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);
}

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

	// Alignment of values as stored by the engine
	{
		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterVec16(engine);
		engine->RegisterObjectType("ref", 0, asOBJ_REF | asOBJ_NOCOUNT);

		if( engine->GetAlignmentOfType(asTYPEID_INT8) != 1 )
			TEST_FAILED;
		if( engine->GetAlignmentOfType(asTYPEID_INT16) != 2 )
			TEST_FAILED;
		if( engine->GetAlignmentOfType(asTYPEID_DOUBLE) != 8 )
			TEST_FAILED;
		if( engine->GetAlignmentOfType(engine->GetTypeIdByDecl("vec16")) != 16 )
			TEST_FAILED;
		// Reference types and handles are stored as pointers
		if( engine->GetAlignmentOfType(engine->GetTypeIdByDecl("ref")) != sizeof(void*) )
			TEST_FAILED;
		if( engine->GetAlignmentOfType(engine->GetTypeIdByDecl("ref@")) != sizeof(void*) )
			TEST_FAILED;
		// Type id 0 is a null handle in list buffers
		if( engine->GetAlignmentOfType(0) != sizeof(void*) )
			TEST_FAILED;

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Members of script classes are naturally aligned, and the class takes the alignment of its most aligned member
	{
		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		r = engine->RegisterObjectType("pod16", (int)sizeof(Pod16), (int)alignof(Pod16), asOBJ_VALUE | asOBJ_POD);
		if( r < 0 )
			TEST_FAILED;
		RegisterVec16(engine);

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test",
			"class Small { int8 a; } \n"
			"class Mixed { int8 a; double b; int8 c; int16 d; int64 e; pod16 f; int8 g; vec16 h; Small @i; } \n"
			"class Derived : Mixed { int8 j; pod16 k; } \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;
		else
		{
			asITypeInfo *small = mod->GetTypeInfoByName("Small");
			asITypeInfo *mixed = mod->GetTypeInfoByName("Mixed");
			asITypeInfo *derived = mod->GetTypeInfoByName("Derived");

			// Without members aligned to more than a pointer the class keeps the alignment of the script object
			if( small->GetAlignment() != alignof(void*) )
				TEST_FAILED;
			if( mixed->GetAlignment() != 16 )
				TEST_FAILED;
			if( derived->GetAlignment() != 16 )
				TEST_FAILED;

			// Each member is placed at an offset that is a multiple of its alignment
			struct { const char *name; int alignment; } members[] =
			{
				{"b", 8}, {"d", 2}, {"e", 8}, {"f", 16}, {"h", (int)sizeof(void*)}, {"i", (int)sizeof(void*)}
			};
			for( asUINT n = 0; n < sizeof(members)/sizeof(members[0]); n++ )
			{
				int offset = PropertyOffset(mixed, members[n].name);
				if( offset < 0 || offset % members[n].alignment != 0 )
				{
					PRINTF("Member '%s' is at offset %d, which is not aligned to %d\n", members[n].name, offset, members[n].alignment);
					TEST_FAILED;
				}
			}

			// No more padding than necessary, the members follow each other closely
			if( PropertyOffset(mixed, "c") != PropertyOffset(mixed, "b") + 8 )
				TEST_FAILED;
			if( PropertyOffset(mixed, "d") != PropertyOffset(mixed, "c") + 2 )
				TEST_FAILED;

			// The derived class keeps the layout of the base class
			const char *baseMembers[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i"};
			for( asUINT n = 0; n < sizeof(baseMembers)/sizeof(baseMembers[0]); n++ )
			{
				if( PropertyOffset(derived, baseMembers[n]) != PropertyOffset(mixed, baseMembers[n]) )
					TEST_FAILED;
			}
			if( PropertyOffset(derived, "k") % 16 != 0 )
				TEST_FAILED;

			// The members are also aligned in memory, as the object is allocated with the class alignment
			asIScriptObject *obj = (asIScriptObject*)engine->CreateScriptObject(derived);
			if( obj == 0 )
				TEST_FAILED;
			else
			{
				for( asUINT n = 0; n < obj->GetPropertyCount(); n++ )
				{
					const char *name = obj->GetPropertyName(n);
					if( (strcmp(name, "f") == 0 || strcmp(name, "k") == 0) && (uintptr_t)obj->GetAddressOfProperty(n) % 16 )
					{
						PRINTF("Member '%s' is not aligned in memory\n", name);
						TEST_FAILED;
					}
				}
				obj->Release();
			}
		}

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Values in initialization lists are naturally aligned, and the list factories find them
	{
		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		RegisterStdString(engine);
		RegisterScriptArray(engine, false);
		RegisterScriptDictionary(engine);
		RegisterVec16(engine);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script =
			"class Obj { int v; Obj(int a) { v = a; } } \n"
			"void main() \n"
			"{ \n"
			"  array<double> d = {1.5, 2.5, 3.5}; \n"
			"  assert( d.length() == 3 && d[0] == 1.5 && d[2] == 3.5 ); \n"
			"  array<int8> b = {1, 2, 3}; \n"
			"  assert( b.length() == 3 && b[2] == 3 ); \n"
			"  array<Obj@> h = {Obj(1), null, Obj(3)}; \n"
			"  assert( h.length() == 3 && h[0].v == 1 && h[1] is null && h[2].v == 3 ); \n"
			"  array<array<double>> n = {{1.5}, {2.5, 3.5}}; \n"
			"  assert( n.length() == 2 && n[1][1] == 3.5 ); \n"
			// Empty elements are default constructed directly in the list buffer
			"  array<vec16> v = {,,}; \n"
			"  assert( v.length() == 3 ); \n"
			// A null in a ? position must not shift the values that follow it
			"  dictionary dict = {{'a', int8(1)}, {'b', null}, {'c', 2.5}, {'d', int16(4)}, {'e', 5.5}}; \n"
			"  assert( int8(dict['a']) == 1 ); \n"
			"  assert( double(dict['c']) == 2.5 ); \n"
			"  assert( int16(dict['d']) == 4 ); \n"
			"  assert( double(dict['e']) == 5.5 ); \n"
			"} \n";

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		g_misalignedVec16 = 0;
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		if( g_misalignedVec16 != 0 )
		{
			PRINTF("%d vec16 instances were not aligned\n", g_misalignedVec16);
			TEST_FAILED;
		}

		// The same must work after saving and loading the bytecode, which recalculates the list layout
		CBytecodeStream stream("test");
		r = mod->SaveByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;
		mod = engine->GetModule("loaded", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;
		r = ExecuteString(engine, "main()", mod);
		if( r != asEXECUTION_FINISHED )
			TEST_FAILED;
		if( g_misalignedVec16 != 0 )
		{
			PRINTF("%d vec16 instances were not aligned after loading\n", g_misalignedVec16);
			TEST_FAILED;
		}

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Local variables, arguments and return values on the script stack are aligned
	{
		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		r = engine->RegisterObjectType("pod16", (int)sizeof(Pod16), (int)alignof(Pod16), asOBJ_VALUE | asOBJ_POD);
		if( r < 0 )
			TEST_FAILED;
		RegisterVec16(engine);
		engine->RegisterGlobalFunction("void checkArgs(int8, double, int, int64, float, const pod16 &in, const vec16 &in)", asFUNCTION(CheckArgs_Generic), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void checkVarArg(const ?&in)", asFUNCTION(CheckVarArg_Generic), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void checkPod(const pod16 &in)", asFUNCTION(CheckPod_Generic), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void checkDouble(double &out)", asFUNCTION(CheckDouble_Generic), asCALL_GENERIC);
		engine->RegisterGlobalFunction("vec16 genVec()", asFUNCTION(GenVec_Generic), asCALL_GENERIC);
		engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(Assert), asCALL_GENERIC);

		const char *script =
			"class Obj { int v; } \n"
			"vec16 makeVec() { vec16 v; return v; } \n"
			// The number and size of the arguments vary between the functions, so the frames
			// start at different positions, which requires the arguments to be moved to align them
			"void level(int depth, int8 pad) \n"
			"{ \n"
			"  int8 a = 1; \n"
			"  vec16 v; \n"
			"  double d = 2; \n"
			"  pod16 p; \n"
			"  checkPod(p); \n"
			"  vec16 r = makeVec(); \n"
			"  vec16 g = genVec(); \n"
			"  checkArgs(1, 2.5, 3, 4, 5.5f, p, v); \n"
			"  checkVarArg(2.5); \n"
			"  double o; \n"
			"  checkDouble(o); \n"
			"  assert( o == 1.5 ); \n"
			"  if( depth > 0 ) \n"
			"  { \n"
			"    if( depth % 2 == 0 ) level(depth - 1, a); \n"
			"    else level2(depth - 1); \n"
			"  } \n"
			"} \n"
			"void level2(int depth) { int8 x = 1; vec16 v; level(depth, x); } \n"
			"void main() { level(10, 0); } \n"
			// Deep recursion moves the frames to new stack blocks
			"void deep(int depth) { vec16 v; if( depth > 0 ) deep(depth - 1); } \n"
			"void runDeep() { deep(500); } \n"
			// Exceptions must clean up the values in the realigned frames
			"void fail(int depth, int8 pad) { vec16 v; if( depth == 0 ) { Obj @o; o.v = 1; } else fail(depth - 1, pad); } \n"
			"void runFail() { try { fail(5, 1); } catch {} fail(3, 1); } \n";

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", script);
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		// Run the same tests before and after saving and loading the bytecode
		CBytecodeStream stream("test");
		for( int pass = 0; pass < 2 && !fail; pass++ )
		{
			if( pass == 1 )
			{
				r = mod->SaveByteCode(&stream);
				if( r < 0 )
					TEST_FAILED;
				mod = engine->GetModule("loaded", asGM_ALWAYS_CREATE);
				r = mod->LoadByteCode(&stream);
				if( r < 0 )
					TEST_FAILED;
			}

			g_misalignedVec16 = 0;
			g_aliveVec16 = 0;
			g_badArgs = 0;

			r = ExecuteString(engine, "main()", mod);
			if( r != asEXECUTION_FINISHED )
				TEST_FAILED;
			r = ExecuteString(engine, "runDeep()", mod);
			if( r != asEXECUTION_FINISHED )
				TEST_FAILED;
			r = ExecuteString(engine, "runFail()", mod);
			if( r != asEXECUTION_EXCEPTION )
				TEST_FAILED;

			// The application calling a script function that returns a value type
			asIScriptContext *ctx = engine->CreateContext();
			r = ctx->Prepare(mod->GetFunctionByName("makeVec"));
			if( r < 0 )
				TEST_FAILED;
			r = ctx->Execute();
			if( r != asEXECUTION_FINISHED )
				TEST_FAILED;
			if( (uintptr_t)ctx->GetReturnObject() % 16 )
				g_misalignedVec16++;
			ctx->Release();

			if( g_misalignedVec16 != 0 )
			{
				PRINTF("%d vec16 instances were not aligned (pass %d)\n", g_misalignedVec16, pass);
				TEST_FAILED;
			}
			if( g_badArgs != 0 )
			{
				PRINTF("%d arguments were not aligned or had wrong values (pass %d)\n", g_badArgs, pass);
				TEST_FAILED;
			}
			if( g_aliveVec16 != 0 )
			{
				PRINTF("%d vec16 instances were not destroyed (pass %d)\n", g_aliveVec16, pass);
				TEST_FAILED;
			}
		}

		if( bout.buffer != "" )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	// Bytecode saved on one platform can be loaded on another, where the value types have different sizes and
	// alignments. This is simulated by registering the types differently in the engine that saves the bytecode
	// and the engine that loads it. The layout of the local variables must be recomputed when loading.
	{
		struct Platform { asUINT valSize, valAlignment, podSize; };
		const Platform platforms[] = { {24, 8, 8}, {48, 16, 32}, {12, 4, 4} };
		const asUINT numPlatforms = sizeof(platforms)/sizeof(platforms[0]);

		const char *script =
			"val makeVal() { val v; return v; } \n"
			"void inner(val v, int8 x) \n"
			"{ \n"
			"  val w = v; \n"
			"  podx q; \n"
			"  double z = 4.5; \n"
			"  checkVal(w); checkPodx(q); \n"
			"  assert( x == 3 && z == 4.5 ); \n"
			"} \n"
			"void main() \n"
			"{ \n"
			"  int8 a = 1; \n"
			"  val v1; \n"
			"  double d1 = 1.5; \n"
			"  podx p1; \n"
			"  int8 b = 2; \n"
			"  val v2 = makeVal(); \n"
			"  int64 i1 = 123456789012; \n"
			"  podx p2; \n"
			"  double d2 = 2.5; \n"
			"  checkVal(v1); checkVal(v2); checkPodx(p1); checkPodx(p2); \n"
			"  v1 = v2; \n"
			"  for( int n = 0; n < 2; n++ ) \n"
			"  { \n"
			"    int8 c = 5; \n"
			"    val v3; \n"
			"    double d3 = 3.5; \n"
			"    checkVal(v3); \n"
			"    assert( c == 5 && d3 == 3.5 ); \n"
			"  } \n"
			"  inner(v1, 3); \n"
			"  assert( a == 1 && b == 2 && d1 == 1.5 && d2 == 2.5 && i1 == 123456789012 ); \n"
			"} \n";

		for( asUINT from = 0; from < numPlatforms; from++ )
		{
			for( asUINT to = 0; to < numPlatforms; to++ )
			{
				if( from == to )
					continue;

				// Build and run on the first platform, then save the bytecode
				asIScriptEngine *engine1 = asCreateScriptEngine();
				engine1->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
				bout.buffer = "";
				RegisterPlatformTypes(engine1, platforms[from].valSize, platforms[from].valAlignment, platforms[from].podSize);
				g_valSize = platforms[from].valSize;
				g_valAlignment = platforms[from].valAlignment;
				g_badVals = 0;

				asIScriptModule *mod = engine1->GetModule("test", asGM_ALWAYS_CREATE);
				mod->AddScriptSection("test", script);
				r = mod->Build();
				if( r < 0 )
					TEST_FAILED;
				r = ExecuteString(engine1, "main()", mod);
				if( r != asEXECUTION_FINISHED )
					TEST_FAILED;

				CBytecodeStream stream("test");
				r = mod->SaveByteCode(&stream);
				if( r < 0 )
					TEST_FAILED;
				engine1->ShutDownAndRelease();

				// Load and run on the second platform
				asIScriptEngine *engine2 = asCreateScriptEngine();
				engine2->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
				RegisterPlatformTypes(engine2, platforms[to].valSize, platforms[to].valAlignment, platforms[to].podSize);
				g_valSize = platforms[to].valSize;
				g_valAlignment = platforms[to].valAlignment;

				mod = engine2->GetModule("test", asGM_ALWAYS_CREATE);
				r = mod->LoadByteCode(&stream);
				if( r < 0 )
					TEST_FAILED;
				else
				{
					r = ExecuteString(engine2, "main()", mod);
					if( r != asEXECUTION_FINISHED )
						TEST_FAILED;
				}
				engine2->ShutDownAndRelease();

				if( g_badVals != 0 )
				{
					PRINTF("%d values were misaligned or overwritten when saving with %d and loading with %d byte alignment\n", g_badVals, platforms[from].valAlignment, platforms[to].valAlignment);
					TEST_FAILED;
				}
				if( bout.buffer != "" )
				{
					PRINTF("%s", bout.buffer.c_str());
					TEST_FAILED;
				}
			}
		}
	}

	// Bytecode that was not saved by a compatible version of the library is rejected
	{
		engine = asCreateScriptEngine();
		engine->SetMessageCallback(asMETHOD(CBufferedOutStream, Callback), &bout, asCALL_THISCALL);
		bout.buffer = "";

		asIScriptModule *mod = engine->GetModule("test", asGM_ALWAYS_CREATE);
		mod->AddScriptSection("test", "void main() {} \n");
		r = mod->Build();
		if( r < 0 )
			TEST_FAILED;

		CBytecodeStream stream("test");
		r = mod->SaveByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		// The same bytecode loads fine
		mod = engine->GetModule("loaded", asGM_ALWAYS_CREATE);
		r = mod->LoadByteCode(&stream);
		if( r < 0 )
			TEST_FAILED;

		// Change the version stored after the magic number
		stream.Restart();
		stream.buffer[4] ^= 0xFF;
		r = mod->LoadByteCode(&stream);
		if( r >= 0 )
			TEST_FAILED;
		if( bout.buffer.find("The bytecode was not saved by a compatible version of the library") == std::string::npos )
		{
			PRINTF("%s", bout.buffer.c_str());
			TEST_FAILED;
		}

		engine->ShutDownAndRelease();
	}

	return fail;
}

} // namespace TestAlignment
