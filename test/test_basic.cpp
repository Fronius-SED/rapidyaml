#include "c4/yml/std/std.hpp"
#include "c4/yml/parse.hpp"
#include "c4/yml/emit.hpp"
#include <c4/format.hpp>
#include <c4/yml/detail/checks.hpp>
#include <c4/yml/detail/print.hpp>

#include "./test_case.hpp"

#include <gtest/gtest.h>

#if defined(_MSC_VER)
#   pragma warning(push)
#   pragma warning(disable: 4389) // signed/unsigned mismatch
#elif defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wdollar-in-identifier-extension"
#elif defined(__GNUC__)
#   pragma GCC diagnostic push
#endif

namespace foo {

template<class T>
struct vec2
{
    T x, y;
};
template<class T>
struct vec3
{
    T x, y, z;
};
template<class T>
struct vec4
{
    T x, y, z, w;
};

template<class T> size_t to_chars(c4::substr buf, vec2<T> v) { return c4::format(buf, "({},{})", v.x, v.y); }
template<class T> size_t to_chars(c4::substr buf, vec3<T> v) { return c4::format(buf, "({},{},{})", v.x, v.y, v.z); }
template<class T> size_t to_chars(c4::substr buf, vec4<T> v) { return c4::format(buf, "({},{},{},{})", v.x, v.y, v.z, v.w); }

template<class T> bool from_chars(c4::csubstr buf, vec2<T> *v) { size_t ret = c4::unformat(buf, "({},{})", v->x, v->y); return ret != c4::yml::npos; }
template<class T> bool from_chars(c4::csubstr buf, vec3<T> *v) { size_t ret = c4::unformat(buf, "({},{},{})", v->x, v->y, v->z); return ret != c4::yml::npos; }
template<class T> bool from_chars(c4::csubstr buf, vec4<T> *v) { size_t ret = c4::unformat(buf, "({},{},{},{})", v->x, v->y, v->z, v->w); return ret != c4::yml::npos; }

TEST(serialize, type_as_str)
{
    c4::yml::Tree t;

    auto r = t.rootref();
    r |= c4::yml::MAP;

    vec2<int> v2in{10, 11};
    vec2<int> v2out{1, 2};
    r["v2"] << v2in;
    r["v2"] >> v2out;
    EXPECT_EQ(v2in.x, v2out.x);
    EXPECT_EQ(v2in.y, v2out.y);

    vec3<int> v3in{100, 101, 102};
    vec3<int> v3out{1, 2, 3};
    r["v3"] << v3in;
    r["v3"] >> v3out;
    EXPECT_EQ(v3in.x, v3out.x);
    EXPECT_EQ(v3in.y, v3out.y);
    EXPECT_EQ(v3in.z, v3out.z);

    vec4<int> v4in{1000, 1001, 1002, 1003};
    vec4<int> v4out{1, 2, 3, 4};
    r["v4"] << v4in;
    r["v4"] >> v4out;
    EXPECT_EQ(v4in.x, v4out.x);
    EXPECT_EQ(v4in.y, v4out.y);
    EXPECT_EQ(v4in.z, v4out.z);
    EXPECT_EQ(v4in.w, v4out.w);

    char buf[256];
    c4::csubstr ret = c4::yml::emit(t, buf);
    EXPECT_EQ(ret, R"(v2: '(10,11)'
v3: '(100,101,102)'
v4: '(1000,1001,1002,1003)'
)");
}
} // namespace foo

namespace c4 { namespace yml {

//-------------------------------------------
Tree get_test_tree()
{
    Tree t = parse("{a: b, c: d, e: [0, 1, 2, 3]}");
    // make sure the tree has strings in its arena
    NodeRef n = t.rootref();
    NodeRef ch = n.append_child();
    ch << key("serialized_key");
    ch << 89;
    return t;
}

//-------------------------------------------
TEST(tree, copy_ctor)
{
    Tree t = get_test_tree();
    {
        Tree cp(t);
        test_invariants(t);
        test_invariants(cp); 
        test_compare(t, cp);
        test_arena_not_shared(t, cp);
    }
}

//-------------------------------------------
TEST(tree, move_ctor)
{
    Tree t = get_test_tree();
    Tree save(t);
    EXPECT_EQ(t.size(), save.size());

    {
        Tree cp(std::move(t));
        EXPECT_EQ(t.size(), 0);
        EXPECT_EQ(save.size(), cp.size());
        test_invariants(t);
        test_invariants(cp);
        test_invariants(save);
        test_compare(cp, save);
        test_arena_not_shared(t, cp);
        test_arena_not_shared(save, cp);
    }
}

//-------------------------------------------
TEST(tree, copy_assign)
{
    Tree t = get_test_tree();
    Tree cp;

    cp = t;
    test_invariants(t);
    test_invariants(cp);
    test_compare(t, cp);
    test_arena_not_shared(t, cp);
}

//-------------------------------------------
TEST(tree, move_assign)
{
    Tree t = get_test_tree();
    Tree cp;
    Tree save(t);
    EXPECT_EQ(t.size(), save.size());

    cp = std::move(t);
    test_invariants(t);
    test_invariants(cp);
    test_invariants(cp);
    test_invariants(save);
    test_compare(save, cp);
    test_arena_not_shared(t, cp);
    test_arena_not_shared(save, cp);
}

//-------------------------------------------
TEST(tree, reserve)
{
    Tree t(16, 64);
    EXPECT_EQ(t.capacity(), 16);
    EXPECT_EQ(t.slack(), 15);
    EXPECT_EQ(t.size(), 1);
    EXPECT_EQ(t.arena_capacity(), 64);
    EXPECT_EQ(t.arena_slack(), 64);
    EXPECT_EQ(t.arena_size(), 0);
    test_invariants(t);
    
    auto buf = t.m_buf;
    t.reserve(16);
    t.reserve_arena(64);
    EXPECT_EQ(t.m_buf, buf);
    EXPECT_EQ(t.capacity(), 16);
    EXPECT_EQ(t.slack(), 15);
    EXPECT_EQ(t.size(), 1);
    EXPECT_EQ(t.arena_capacity(), 64);
    EXPECT_EQ(t.arena_slack(), 64);
    EXPECT_EQ(t.arena_size(), 0);
    test_invariants(t);
    
    t.reserve(32);
    t.reserve_arena(128);
    EXPECT_EQ(t.capacity(), 32);
    EXPECT_EQ(t.slack(), 31);
    EXPECT_EQ(t.size(), 1);
    EXPECT_EQ(t.arena_capacity(), 128);
    EXPECT_EQ(t.arena_slack(), 128);
    EXPECT_EQ(t.arena_size(), 0);
    test_invariants(t);
    
    buf = t.m_buf;
    parse("[a, b, c, d, e, f]", &t);
    EXPECT_EQ(t.m_buf, buf);
    EXPECT_EQ(t.capacity(), 32);
    EXPECT_EQ(t.slack(), 25);
    EXPECT_EQ(t.size(), 7);
    EXPECT_EQ(t.arena_capacity(), 128);
    EXPECT_EQ(t.arena_slack(), 110);
    EXPECT_EQ(t.arena_size(), 18);
    test_invariants(t);
    
    t.reserve(64);
    t.reserve_arena(256);
    EXPECT_EQ(t.capacity(), 64);
    EXPECT_EQ(t.slack(), 57);
    EXPECT_EQ(t.size(), 7);
    EXPECT_EQ(t.arena_capacity(), 256);
    EXPECT_EQ(t.arena_slack(), 238);
    EXPECT_EQ(t.arena_size(), 18);
    test_invariants(t);
}

TEST(tree, clear)
{
    Tree t(16, 64);
    EXPECT_EQ(t.capacity(), 16);
    EXPECT_EQ(t.slack(), 15);
    EXPECT_EQ(t.size(), 1);
    EXPECT_EQ(t.arena_capacity(), 64);
    EXPECT_EQ(t.arena_slack(), 64);
    EXPECT_EQ(t.arena_size(), 0);
    test_invariants(t);

    t.clear();
    t.clear_arena();
    EXPECT_EQ(t.capacity(), 16);
    EXPECT_EQ(t.slack(), 15);
    EXPECT_EQ(t.size(), 1);
    EXPECT_EQ(t.arena_capacity(), 64);
    EXPECT_EQ(t.arena_slack(), 64);
    EXPECT_EQ(t.arena_size(), 0);
    test_invariants(t);
    
    auto buf = t.m_buf;
    t.reserve(16);
    t.reserve_arena(64);
    EXPECT_EQ(t.m_buf, buf);
    EXPECT_EQ(t.capacity(), 16);
    EXPECT_EQ(t.slack(), 15);
    EXPECT_EQ(t.size(), 1);
    EXPECT_EQ(t.arena_capacity(), 64);
    EXPECT_EQ(t.arena_slack(), 64);
    EXPECT_EQ(t.arena_size(), 0);
    test_invariants(t);
    
    t.reserve(32);
    t.reserve_arena(128);
    EXPECT_EQ(t.capacity(), 32);
    EXPECT_EQ(t.slack(), 31);
    EXPECT_EQ(t.size(), 1);
    EXPECT_EQ(t.arena_capacity(), 128);
    EXPECT_EQ(t.arena_slack(), 128);
    EXPECT_EQ(t.arena_size(), 0);
    test_invariants(t);
    
    buf = t.m_buf;
    parse("[a, b, c, d, e, f]", &t);
    EXPECT_EQ(t.m_buf, buf);
    EXPECT_EQ(t.capacity(), 32);
    EXPECT_EQ(t.slack(), 25);
    EXPECT_EQ(t.size(), 7);
    EXPECT_EQ(t.arena_capacity(), 128);
    EXPECT_EQ(t.arena_slack(), 110);
    EXPECT_EQ(t.arena_size(), 18);
    test_invariants(t);

    t.clear();
    t.clear_arena();
    EXPECT_EQ(t.capacity(), 32);
    EXPECT_EQ(t.slack(), 31);
    EXPECT_EQ(t.size(), 1);
    EXPECT_EQ(t.arena_capacity(), 128);
    EXPECT_EQ(t.arena_slack(), 128);
    EXPECT_EQ(t.arena_size(), 0);
    test_invariants(t);
}


//-------------------------------------------
TEST(tree, operator_square_brackets)
{
    {
        Tree t = parse("[0, 1, 2, 3, 4]");
        Tree &m = t;
        Tree const& cm = t;
        EXPECT_EQ(m[0].val(), "0");
        EXPECT_EQ(m[1].val(), "1");
        EXPECT_EQ(m[2].val(), "2");
        EXPECT_EQ(m[3].val(), "3");
        EXPECT_EQ(m[4].val(), "4");
        EXPECT_EQ(cm[0].val(), "0");
        EXPECT_EQ(cm[1].val(), "1");
        EXPECT_EQ(cm[2].val(), "2");
        EXPECT_EQ(cm[3].val(), "3");
        EXPECT_EQ(cm[4].val(), "4");
        //
        EXPECT_TRUE(m[0]  == "0");
        EXPECT_TRUE(m[1]  == "1");
        EXPECT_TRUE(m[2]  == "2");
        EXPECT_TRUE(m[3]  == "3");
        EXPECT_TRUE(m[4]  == "4");
        EXPECT_TRUE(cm[0] == "0");
        EXPECT_TRUE(cm[1] == "1");
        EXPECT_TRUE(cm[2] == "2");
        EXPECT_TRUE(cm[3] == "3");
        EXPECT_TRUE(cm[4] == "4");
        //
        EXPECT_FALSE(m[0]  != "0");
        EXPECT_FALSE(m[1]  != "1");
        EXPECT_FALSE(m[2]  != "2");
        EXPECT_FALSE(m[3]  != "3");
        EXPECT_FALSE(m[4]  != "4");
        EXPECT_FALSE(cm[0] != "0");
        EXPECT_FALSE(cm[1] != "1");
        EXPECT_FALSE(cm[2] != "2");
        EXPECT_FALSE(cm[3] != "3");
        EXPECT_FALSE(cm[4] != "4");
    }
    {
        Tree t = parse("{a: 0, b: 1, c: 2, d: 3, e: 4}");
        Tree &m = t;
        Tree const& cm = t;
        EXPECT_EQ(m["a"].val(), "0");
        EXPECT_EQ(m["b"].val(), "1");
        EXPECT_EQ(m["c"].val(), "2");
        EXPECT_EQ(m["d"].val(), "3");
        EXPECT_EQ(m["e"].val(), "4");
        EXPECT_EQ(cm["a"].val(), "0");
        EXPECT_EQ(cm["b"].val(), "1");
        EXPECT_EQ(cm["c"].val(), "2");
        EXPECT_EQ(cm["d"].val(), "3");
        EXPECT_EQ(cm["e"].val(), "4");
        //
        EXPECT_TRUE(m["a"] == "0");
        EXPECT_TRUE(m["b"] == "1");
        EXPECT_TRUE(m["c"] == "2");
        EXPECT_TRUE(m["d"] == "3");
        EXPECT_TRUE(m["e"] == "4");
        EXPECT_TRUE(cm["a"] == "0");
        EXPECT_TRUE(cm["b"] == "1");
        EXPECT_TRUE(cm["c"] == "2");
        EXPECT_TRUE(cm["d"] == "3");
        EXPECT_TRUE(cm["e"] == "4");
        //
        EXPECT_FALSE(m["a"] != "0");
        EXPECT_FALSE(m["b"] != "1");
        EXPECT_FALSE(m["c"] != "2");
        EXPECT_FALSE(m["d"] != "3");
        EXPECT_FALSE(m["e"] != "4");
        EXPECT_FALSE(cm["a"] != "0");
        EXPECT_FALSE(cm["b"] != "1");
        EXPECT_FALSE(cm["c"] != "2");
        EXPECT_FALSE(cm["d"] != "3");
        EXPECT_FALSE(cm["e"] != "4");
    }
}

TEST(tree, relocate)
{
    // create a tree with anchors and refs, and copy it to ensure the
    // relocation also applies to the anchors and refs. Ensure to put
    // the source in the arena so that it gets relocated.
    Tree tree = parse(R"(&keyanchor key: val
key2: &valanchor val2
keyref: *keyanchor
*valanchor: was val anchor
!!int 0: !!str foo
!!str doe: !!str a deer a female deer
ray: a drop of golden sun
me: a name I call myself
far: a long long way to run
)");
    Tree copy = tree;
    EXPECT_EQ(copy.size(), tree.size());
    EXPECT_EQ(emitrs<std::string>(copy), R"(&keyanchor key: val
key2: &valanchor val2
keyref: *keyanchor
*valanchor: was val anchor
!!int 0: !!str foo
!!str doe: !!str a deer a female deer
ray: a drop of golden sun
me: a name I call myself
far: a long long way to run
)");
    //
    Tree copy2 = copy;
    EXPECT_EQ(copy.size(), tree.size());
    copy2.resolve();
    EXPECT_EQ(emitrs<std::string>(copy2), R"(key: val
key2: val2
keyref: key
val2: was val anchor
!!int 0: !!str foo
!!str doe: !!str a deer a female deer
ray: a drop of golden sun
me: a name I call myself
far: a long long way to run
)");
}


//-------------------------------------------
template<class Container, class... Args>
void do_test_serialize(Args&& ...args)
{
    using namespace c4::yml;
    Container s(std::forward<Args>(args)...);
    Container out;

    Tree t;
    NodeRef n(&t);

    n << s;
    //print_tree(t);
    emit(t);
    c4::yml::check_invariants(t);
    n >> out;
    EXPECT_EQ(s, out);
}


TEST(serialize, std_vector_int)
{
    using T = int;
    using L = std::initializer_list<T>;
    do_test_serialize<std::vector<T>>(L{1, 2, 3, 4, 5});
}
TEST(serialize, std_vector_string)
{
    using T = std::string;
    using L = std::initializer_list<T>;
    do_test_serialize<std::vector<T>>(L{"0asdadk0", "1sdfkjdfgu1", "2fdfdjkhdfgkjhdfi2", "3e987dfgnfdg83", "4'd0fgºçdfg«4"});
}
TEST(serialize, std_vector_std_vector_int)
{
    using T = std::vector<int>;
    using L = std::initializer_list<T>;
    do_test_serialize<std::vector<T>>(L{{1, 2, 3, 4, 5}, {6, 7, 8, 9, 0}});
}


TEST(serialize, std_map__int_int)
{
    using M = std::map<int, int>;
    using L = std::initializer_list<typename M::value_type>;
    do_test_serialize<M>(L{{10, 0}, {11, 1}, {22, 2}, {10001, 1000}, {20002, 2000}, {30003, 3000}});
}
TEST(serialize, std_map__std_string_int)
{
    using M = std::map<std::string, int>;
    using L = std::initializer_list<typename M::value_type>;
    do_test_serialize<M>(L{{"asdsdf", 0}, {"dfgdfgdfg", 1}, {"dfgjdfgkjh", 2}});
}
TEST(serialize, std_map__string_vectori)
{
    using M = std::map<std::string, std::vector<int>>;
    using L = std::initializer_list<typename M::value_type>;
    do_test_serialize<M>(L{{"asdsdf", {0, 1, 2, 3}}, {"dfgdfgdfg", {4, 5, 6, 7}}, {"dfgjdfgkjh", {8, 9, 10, 11}}});
}
TEST(serialize, std_vector__map_string_int)
{
    using V = std::vector< std::map<std::string, int>>;
    using M = typename V::value_type;
    using L = std::initializer_list<M>;
    do_test_serialize<V>(L{
            M{{"asdasf",  0}, {"dfgkjhdfg",  1}, {"fghffg",  2}, {"r5656kjnh9b'dfgwg+*",  3}},
            M{{"asdasf", 10}, {"dfgkjhdfg", 11}, {"fghffg", 12}, {"r5656kjnh9b'dfgwg+*", 13}},
            M{{"asdasf", 20}, {"dfgkjhdfg", 21}, {"fghffg", 22}, {"r5656kjnh9b'dfgwg+*", 23}},
            M{{"asdasf", 30}, {"dfgkjhdfg", 31}, {"fghffg", 32}, {"r5656kjnh9b'dfgwg+*", 33}},
    });
}


TEST(serialize, bool)
{
    Tree t = parse("{a: 0, b: false, c: 1, d: true}");
    bool v, w;
    t["a"] >> v;
    EXPECT_EQ(v, false);
    t["b"] >> v;
    EXPECT_EQ(v, false);
    t["c"] >> v;
    EXPECT_EQ(v, true);
    t["d"] >> v;
    EXPECT_EQ(v, true);

    t["e"] << true;
    EXPECT_EQ(t["e"].val(), "1");
    t["e"] >> w;
    EXPECT_EQ(w, true);

    t["e"] << false;
    EXPECT_EQ(t["e"].val(), "0");
    t["e"] >> w;
    EXPECT_EQ(w, false);

    t["e"] << fmt::boolalpha(true);
    EXPECT_EQ(t["e"].val(), "true");
    t["e"] >> w;
    EXPECT_EQ(w, true);

    t["e"] << fmt::boolalpha(false);
    EXPECT_EQ(t["e"].val(), "false");
    t["e"] >> w;
    EXPECT_EQ(w, false);
}

TEST(serialize, nan)
{
    Tree t = parse(R"(
good:
  - .nan
  -   .nan
  -
   .nan
set:
  - nothing
  - nothing
})");
    t["set"][0] << std::numeric_limits<float>::quiet_NaN();
    t["set"][1] << std::numeric_limits<double>::quiet_NaN();
    EXPECT_EQ(t["set"][0].val(), ".nan");
    EXPECT_EQ(t["set"][1].val(), ".nan");
    EXPECT_EQ(t["good"][0].val(), ".nan");
    EXPECT_EQ(t["good"][1].val(), ".nan");
    EXPECT_EQ(t["good"][2].val(), ".nan");
    float f;
    double d;
    f = 0.f;
    d = 0.;
    t["good"][0] >> f;
    t["good"][0] >> d;
    EXPECT_TRUE(std::isnan(f));
    EXPECT_TRUE(std::isnan(d));
    f = 0.f;
    d = 0.;
    t["good"][1] >> f;
    t["good"][1] >> d;
    EXPECT_TRUE(std::isnan(f));
    EXPECT_TRUE(std::isnan(d));
    f = 0.f;
    d = 0.;
    t["good"][2] >> f;
    t["good"][2] >> d;
    EXPECT_TRUE(std::isnan(f));
    EXPECT_TRUE(std::isnan(d));
}

TEST(serialize, inf)
{
    C4_SUPPRESS_WARNING_GCC_CLANG_WITH_PUSH("-Wfloat-equal");
    Tree t = parse(R"(
good:
  - .inf
  -   .inf
  -
   .inf
set:
  - nothing
  - nothing
})");
    float finf = std::numeric_limits<float>::infinity();
    double dinf = std::numeric_limits<double>::infinity();
    t["set"][0] << finf;
    t["set"][1] << dinf;
    EXPECT_EQ(t["set"][0].val(), ".inf");
    EXPECT_EQ(t["set"][1].val(), ".inf");
    EXPECT_EQ(t["good"][0].val(), ".inf");
    EXPECT_EQ(t["good"][1].val(), ".inf");
    EXPECT_EQ(t["good"][2].val(), ".inf");
    float f;
    double d;
    f = 0.f;
    d = 0.;
    t["good"][0] >> f;
    t["good"][0] >> d;
    EXPECT_TRUE(f == finf);
    EXPECT_TRUE(d == dinf);
    f = 0.f;
    d = 0.;
    t["good"][1] >> f;
    t["good"][1] >> d;
    EXPECT_TRUE(f == finf);
    EXPECT_TRUE(d == dinf);
    f = 0.f;
    d = 0.;
    t["good"][2] >> f;
    t["good"][2] >> d;
    EXPECT_TRUE(f == finf);
    EXPECT_TRUE(d == dinf);

    t = parse(R"(
good:
  - -.inf
  -   -.inf
  -
   -.inf
set:
  - nothing
  - nothing
})");
    t["set"][0] << -finf;
    t["set"][1] << -dinf;
    EXPECT_EQ(t["set"][0].val(), "-.inf");
    EXPECT_EQ(t["set"][1].val(), "-.inf");
    EXPECT_EQ(t["good"][0].val(), "-.inf");
    EXPECT_EQ(t["good"][1].val(), "-.inf");
    EXPECT_EQ(t["good"][2].val(), "-.inf");
    f = 0.f;
    d = 0.;
    t["good"][0] >> f;
    t["good"][0] >> d;
    EXPECT_TRUE(f == -finf);
    EXPECT_TRUE(d == -dinf);
    f = 0.f;
    d = 0.;
    t["good"][1] >> f;
    t["good"][1] >> d;
    EXPECT_TRUE(f == -finf);
    EXPECT_TRUE(d == -dinf);
    f = 0.f;
    d = 0.;
    t["good"][2] >> f;
    t["good"][2] >> d;
    EXPECT_TRUE(f == -finf);
    EXPECT_TRUE(d == -dinf);
    C4_SUPPRESS_WARNING_GCC_CLANG_POP
}

TEST(serialize, std_string)
{
    auto t = parse("{foo: bar}");
    std::string s;
    EXPECT_NE(s, "bar");
    t["foo"] >> s;
    EXPECT_EQ(s, "bar");
}

TEST(serialize, anchor_and_ref_round_tree)
{
    const char yaml[] = R"(anchor_objects:
  - &id001
    name: id001
  - &id002
    name: id002
  - name: id003
  - &id004
    name: id004
references:
  reference_key: *id001
  reference_list:
    - *id002
    - *id004
)";

    Tree t = parse(yaml);
    std::string cmpbuf;
    emitrs(t, &cmpbuf);
    EXPECT_EQ(cmpbuf, yaml);
}

TEST(serialize, create_anchor_ref_tree)
{
    const char expected_yaml[] = R"(anchor_objects:
  - &id001
    name: a_name
reference_list:
  - *id001
)";

    Tree tree;
    auto root_id = tree.root_id();
    tree.to_map(root_id);

    auto anchor_list_id = tree.append_child(root_id);
    tree.to_seq(anchor_list_id, "anchor_objects");

    auto anchor_map0 = tree.append_child(anchor_list_id);
    tree.to_map(anchor_map0);
    tree.set_val_anchor(anchor_map0, "id001");

    auto anchor_elem0 = tree.append_child(anchor_map0);
    tree.to_keyval(anchor_elem0, "name", "a_name");

    auto ref_list_id = tree.append_child(root_id);
    tree.to_seq(ref_list_id, "reference_list");

    auto elem0_id = tree.append_child(ref_list_id);
    tree.set_val_ref(elem0_id, "id001");

    std::string cmpbuf;
    emitrs(tree, &cmpbuf);
    EXPECT_EQ(cmpbuf, expected_yaml);
}

} /*namespace yml*/
} /*namespace c4*/

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

namespace c4 {
namespace yml {

void node_scalar_test_empty(NodeScalar const& s)
{
    EXPECT_TRUE(s.empty());
    EXPECT_EQ(s.tag, "");
    EXPECT_EQ(s.tag.len, 0u);
    EXPECT_TRUE(s.tag.empty());
    EXPECT_EQ(s.scalar, "");
    EXPECT_EQ(s.scalar.len, 0u);
    EXPECT_TRUE(s.scalar.empty());
}

void node_scalar_test_foo(NodeScalar const& s, bool with_tag=false)
{
    EXPECT_FALSE(s.empty());
    if(with_tag)
    {
        EXPECT_EQ(s.tag, "!!str");
        EXPECT_EQ(s.tag.len, 5u);
        EXPECT_FALSE(s.tag.empty());
    }
    else
    {
        EXPECT_EQ(s.tag, "");
        EXPECT_EQ(s.tag.len, 0u);
        EXPECT_TRUE(s.tag.empty());
    }
    EXPECT_EQ(s.scalar, "foo");
    EXPECT_EQ(s.scalar.len, 3u);
    EXPECT_FALSE(s.scalar.empty());
}

void node_scalar_test_foo3(NodeScalar const& s, bool with_tag=false)
{
    EXPECT_FALSE(s.empty());
    if(with_tag)
    {
        EXPECT_EQ(s.tag, "!!str+++");
        EXPECT_EQ(s.tag.len, 8u);
        EXPECT_FALSE(s.tag.empty());
    }
    else
    {
        EXPECT_EQ(s.tag, "");
        EXPECT_EQ(s.tag.len, 0u);
        EXPECT_TRUE(s.tag.empty());
    }
    EXPECT_EQ(s.scalar, "foo3");
    EXPECT_EQ(s.scalar.len, 4u);
    EXPECT_FALSE(s.scalar.empty());
}

TEST(NodeScalar, ctor_empty)
{
    NodeScalar s;
    node_scalar_test_empty(s);
}

TEST(NodeScalar, ctor__untagged)
{
    {
        const char sarr[] = "foo";
        const char *sptr = "foo";
        csubstr ssp = "foo";

        for(auto s : {NodeScalar(sarr), NodeScalar(to_csubstr(sptr)), NodeScalar(ssp)})
        {
            node_scalar_test_foo(s);
        }

        NodeScalar s;
        s = {sarr};
        node_scalar_test_foo(s);
        s = to_csubstr(sptr);
        node_scalar_test_foo(s);
        s = {ssp};
        node_scalar_test_foo(s);
    }

    {
        const char sarr[] = "foo3";
        const char *sptr = "foo3";
        csubstr ssp = "foo3";

        for(auto s : {NodeScalar(sarr), NodeScalar(to_csubstr(sptr)), NodeScalar(ssp)})
        {
            node_scalar_test_foo3(s);
        }

        NodeScalar s;
        {
            SCOPED_TRACE("here 1");
            s = {sarr};
            node_scalar_test_foo3(s);
        }
        {
            SCOPED_TRACE("here 2");
            s = to_csubstr(sptr);
            node_scalar_test_foo3(s);
        }
        {
            SCOPED_TRACE("here 3");
            s = ssp;
            node_scalar_test_foo3(s);
        }
    }
}

TEST(NodeScalar, ctor__tagged)
{
    {
        const char sarr[] = "foo", tarr[] = "!!str";
        const char *sptr = "foo";
        const char *tptr = "!!str";
        csubstr ssp = "foo", tsp = "!!str";

        for(NodeScalar s : {
                NodeScalar(tsp, ssp),
                    NodeScalar(tsp, to_csubstr(sptr)),
                    NodeScalar(tsp, sarr),
                NodeScalar(to_csubstr(tptr), ssp),
                    NodeScalar(to_csubstr(tptr), to_csubstr(sptr)),
                    NodeScalar(to_csubstr(tptr), sarr),
                NodeScalar(tarr, ssp),
                    NodeScalar(tarr, to_csubstr(sptr)),
                    NodeScalar(tarr, sarr),
        })
        {
            node_scalar_test_foo(s, true);
        }

        NodeScalar s;

        {
            SCOPED_TRACE("here 0.0");
            s = {tsp, ssp};
            node_scalar_test_foo(s, true);
        }
        {
            SCOPED_TRACE("here 0.1");
            s = {tsp, to_csubstr(sptr)};
            node_scalar_test_foo(s, true);
        }
        {
            SCOPED_TRACE("here 0.2");
            s = {tsp, sarr};
            node_scalar_test_foo(s, true);
        }

        {
            SCOPED_TRACE("here 1.0");
            s = {to_csubstr(tptr), ssp};
            node_scalar_test_foo(s, true);
        }
        {
            SCOPED_TRACE("here 1.1");
            s = {to_csubstr(tptr), to_csubstr(sptr)};
            node_scalar_test_foo(s, true);
        }
        {
            SCOPED_TRACE("here 1.3");
            s = {to_csubstr(tptr), sarr};
            node_scalar_test_foo(s, true);
        }

        {
            SCOPED_TRACE("here 3.0");
            s = {tarr, ssp};
            node_scalar_test_foo(s, true);
        }
        {
            SCOPED_TRACE("here 3.1");
            s = {tarr, to_csubstr(sptr)};
            node_scalar_test_foo(s, true);
        }
        {
            SCOPED_TRACE("here 3.3");
            s = {tarr, sarr};
            node_scalar_test_foo(s, true);
        }

    }

    {
        const char sarr[] = "foo3", tarr[] = "!!str+++";
        const char *sptr = "foo3";
        const char *tptr = "!!str+++";
        csubstr ssp = "foo3", tsp = "!!str+++";

        NodeScalar wtf = {tsp, ssp};
        EXPECT_EQ(wtf.tag, tsp);
        EXPECT_EQ(wtf.scalar, ssp);
        for(auto s : {
                NodeScalar(tsp, ssp),
                    NodeScalar(tsp, to_csubstr(sptr)),
                    NodeScalar(tsp, sarr),
                NodeScalar(to_csubstr(tptr), ssp),
                    NodeScalar(to_csubstr(tptr), to_csubstr(sptr)),
                    NodeScalar(to_csubstr(tptr), sarr),
                NodeScalar(tarr, ssp),
                    NodeScalar(tarr, to_csubstr(sptr)),
                    NodeScalar(tarr, sarr),
        })
        {
            node_scalar_test_foo3(s, true);
        }

        NodeScalar s;

        {
            SCOPED_TRACE("here 0.0");
            s = {tsp, ssp};
            node_scalar_test_foo3(s, true);
        }
        {
            SCOPED_TRACE("here 0.1");
            s = {tsp, to_csubstr(sptr)};
            node_scalar_test_foo3(s, true);
        }
        {
            SCOPED_TRACE("here 0.3");
            s = {tsp, sarr};
            node_scalar_test_foo3(s, true);
        }

        {
            SCOPED_TRACE("here 1.0");
            s = {to_csubstr(tptr), ssp};
            node_scalar_test_foo3(s, true);
        }
        {
            SCOPED_TRACE("here 1.1");
            s = {to_csubstr(tptr), to_csubstr(sptr)};
            node_scalar_test_foo3(s, true);
        }
        {
            SCOPED_TRACE("here 1.3");
            s = {to_csubstr(tptr), sarr};
            node_scalar_test_foo3(s, true);
        }

        {
            SCOPED_TRACE("here 3.0");
            s = {tarr, ssp};
            node_scalar_test_foo3(s, true);
        }
        {
            SCOPED_TRACE("here 3.1");
            s = {tarr, to_csubstr(sptr)};
            node_scalar_test_foo3(s, true);
        }
        {
            SCOPED_TRACE("here 3.3");
            s = {tarr, sarr};
            node_scalar_test_foo3(s, true);
        }

    }

}


TEST(NodeType, type_str)
{
    // avoid coverage misses
    EXPECT_EQ(to_csubstr(NodeType(KEYVAL).type_str()), "KEYVAL");
    EXPECT_EQ(to_csubstr(NodeType(VAL).type_str()), "VAL");
    EXPECT_EQ(to_csubstr(NodeType(MAP).type_str()), "MAP");
    EXPECT_EQ(to_csubstr(NodeType(SEQ).type_str()), "SEQ");
    EXPECT_EQ(to_csubstr(NodeType(KEYMAP).type_str()), "KEYMAP");
    EXPECT_EQ(to_csubstr(NodeType(KEYSEQ).type_str()), "KEYSEQ");
    EXPECT_EQ(to_csubstr(NodeType(DOCSEQ).type_str()), "DOCSEQ");
    EXPECT_EQ(to_csubstr(NodeType(DOCMAP).type_str()), "DOCMAP");
    EXPECT_EQ(to_csubstr(NodeType(DOCVAL).type_str()), "DOCVAL");
    EXPECT_EQ(to_csubstr(NodeType(DOC).type_str()), "DOC");
    EXPECT_EQ(to_csubstr(NodeType(STREAM).type_str()), "STREAM");
    EXPECT_EQ(to_csubstr(NodeType(NOTYPE).type_str()), "NOTYPE");
    EXPECT_EQ(to_csubstr(NodeType(KEYVAL|KEYREF).type_str()), "KEYVAL***");
    EXPECT_EQ(to_csubstr(NodeType(KEYVAL|VALREF).type_str()), "KEYVAL***");
    EXPECT_EQ(to_csubstr(NodeType(KEYVAL|KEYANCH).type_str()), "KEYVAL***");
    EXPECT_EQ(to_csubstr(NodeType(KEYVAL|VALANCH).type_str()), "KEYVAL***");
    EXPECT_EQ(to_csubstr(NodeType(KEYVAL|KEYREF|VALANCH).type_str()), "KEYVAL***");
    EXPECT_EQ(to_csubstr(NodeType(KEYVAL|KEYANCH|VALREF).type_str()), "KEYVAL***");
    EXPECT_EQ(to_csubstr(NodeType(KEYMAP|KEYREF).type_str()), "KEYMAP***");
    EXPECT_EQ(to_csubstr(NodeType(KEYMAP|VALREF).type_str()), "KEYMAP***");
    EXPECT_EQ(to_csubstr(NodeType(KEYMAP|KEYANCH).type_str()), "KEYMAP***");
    EXPECT_EQ(to_csubstr(NodeType(KEYMAP|VALANCH).type_str()), "KEYMAP***");
    EXPECT_EQ(to_csubstr(NodeType(KEYMAP|KEYREF|VALANCH).type_str()), "KEYMAP***");
    EXPECT_EQ(to_csubstr(NodeType(KEYMAP|KEYANCH|VALREF).type_str()), "KEYMAP***");
    EXPECT_EQ(to_csubstr(NodeType(KEYSEQ|KEYREF).type_str()), "KEYSEQ***");
    EXPECT_EQ(to_csubstr(NodeType(KEYSEQ|VALREF).type_str()), "KEYSEQ***");
    EXPECT_EQ(to_csubstr(NodeType(KEYSEQ|KEYANCH).type_str()), "KEYSEQ***");
    EXPECT_EQ(to_csubstr(NodeType(KEYSEQ|VALANCH).type_str()), "KEYSEQ***");
    EXPECT_EQ(to_csubstr(NodeType(KEYSEQ|KEYREF|VALANCH).type_str()), "KEYSEQ***");
    EXPECT_EQ(to_csubstr(NodeType(KEYSEQ|KEYANCH|VALREF).type_str()), "KEYSEQ***");
    EXPECT_EQ(to_csubstr(NodeType(DOCSEQ|VALANCH).type_str()), "DOCSEQ***");
    EXPECT_EQ(to_csubstr(NodeType(DOCSEQ|VALREF).type_str()), "DOCSEQ***");
    EXPECT_EQ(to_csubstr(NodeType(DOCMAP|VALANCH).type_str()), "DOCMAP***");
    EXPECT_EQ(to_csubstr(NodeType(DOCMAP|VALREF).type_str()), "DOCMAP***");
    EXPECT_EQ(to_csubstr(NodeType(DOCVAL|VALANCH).type_str()), "DOCVAL***");
    EXPECT_EQ(to_csubstr(NodeType(DOCVAL|VALREF).type_str()), "DOCVAL***");
    EXPECT_EQ(to_csubstr(NodeType(KEY|KEYREF).type_str()), "KEY***");
    EXPECT_EQ(to_csubstr(NodeType(KEY|KEYANCH).type_str()), "KEY***");
    EXPECT_EQ(to_csubstr(NodeType(VAL|VALREF).type_str()), "VAL***");
    EXPECT_EQ(to_csubstr(NodeType(VAL|VALANCH).type_str()), "VAL***");
    EXPECT_EQ(to_csubstr(NodeType(MAP|VALREF).type_str()), "MAP***");
    EXPECT_EQ(to_csubstr(NodeType(MAP|VALANCH).type_str()), "MAP***");
    EXPECT_EQ(to_csubstr(NodeType(SEQ|VALREF).type_str()), "SEQ***");
    EXPECT_EQ(to_csubstr(NodeType(SEQ|VALANCH).type_str()), "SEQ***");
    EXPECT_EQ(to_csubstr(NodeType(DOC|VALREF).type_str()), "DOC***");
    EXPECT_EQ(to_csubstr(NodeType(DOC|VALANCH).type_str()), "DOC***");
    EXPECT_EQ(to_csubstr(NodeType(KEYREF).type_str()), "(unk)");
    EXPECT_EQ(to_csubstr(NodeType(VALREF).type_str()), "(unk)");
    EXPECT_EQ(to_csubstr(NodeType(KEYANCH).type_str()), "(unk)");
    EXPECT_EQ(to_csubstr(NodeType(VALANCH).type_str()), "(unk)");
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
TEST(NodeInit, ctor__empty)
{
    NodeInit n;
    EXPECT_EQ((type_bits)n.type, (type_bits)NOTYPE);
    EXPECT_EQ(n.key.scalar, "");
    EXPECT_EQ(n.key.tag, "");
    EXPECT_EQ(n.val.scalar, "");
    EXPECT_EQ(n.val.tag, "");
}

TEST(NodeInit, ctor__type_only)
{
    for(auto k : {KEY, KEYVAL, MAP, KEYMAP, SEQ, KEYSEQ})
    {
        SCOPED_TRACE(NodeType::type_str(k));
        NodeInit n(k);
        EXPECT_EQ((type_bits)n.type, (type_bits)k);
        EXPECT_EQ(n.key.scalar, "");
        EXPECT_EQ(n.key.tag, "");
        EXPECT_EQ(n.val.scalar, "");
        EXPECT_EQ(n.val.tag, "");
    }
}

TEST(NodeInit, ctor__val_only)
{
    {
        const char sarr[] = "foo";
        const char *sptr = "foo"; size_t sptrlen = 3;
        csubstr ssp = "foo";

        {
            SCOPED_TRACE("here 0");
            {
                NodeInit s(sarr);
                node_scalar_test_foo(s.val);
                node_scalar_test_empty(s.key);
                s.clear();
            }
            {
                NodeInit s{to_csubstr(sptr)};
                node_scalar_test_foo(s.val);
                node_scalar_test_empty(s.key);
                s.clear();
            }
            {
                NodeInit s{sarr};
                node_scalar_test_foo(s.val);
                node_scalar_test_empty(s.key);
                s.clear();
            }
        }

        {
            SCOPED_TRACE("here 1");
            {
                NodeInit s(sarr);
                node_scalar_test_foo(s.val);
                node_scalar_test_empty(s.key);
                s.clear();
            }
            {
                NodeInit s(to_csubstr(sptr));
                node_scalar_test_foo(s.val);
                node_scalar_test_empty(s.key);
                s.clear();
            }
            {
                NodeInit s(sarr);
                node_scalar_test_foo(s.val);
                node_scalar_test_empty(s.key);
                s.clear();
            }
        }

        {
            SCOPED_TRACE("here 2");
            NodeInit s;
            s = {sarr};
            node_scalar_test_foo(s.val);
            node_scalar_test_empty(s.key);
            s.clear();
            s = {to_csubstr(sptr)};
            node_scalar_test_foo(s.val);
            node_scalar_test_empty(s.key);
            s.clear();
            //s = {sptr, sptrlen}; // fails to compile
            //node_scalar_test_foo(s.val);
            //node_scalar_test_empty(s.key);
            //s.clear();
            s = {ssp};
            node_scalar_test_foo(s.val);
            node_scalar_test_empty(s.key);
            s.clear();
        }

        for(auto s : {
            NodeInit(sarr),
            NodeInit(to_csubstr(sptr)),
            NodeInit(csubstr{sptr, sptrlen}),
            NodeInit(ssp)})
        {
            SCOPED_TRACE("here LOOP");
            node_scalar_test_foo(s.val);
            node_scalar_test_empty(s.key);
        }
    }

    {
        const char sarr[] = "foo3";
        const char *sptr = "foo3"; size_t sptrlen = 4;
        csubstr ssp = "foo3";

        {
            SCOPED_TRACE("here 0");
            NodeInit s = {sarr};
            node_scalar_test_foo3(s.val);
            node_scalar_test_empty(s.key);
        }
        {   // FAILS
            SCOPED_TRACE("here 1");
            //NodeInit s = sarr;
            //node_scalar_test_foo3(s.val);
            //node_scalar_test_empty(s.key);
        }
        {
            SCOPED_TRACE("here 2");
            NodeInit s{sarr};
            node_scalar_test_foo3(s.val);
            node_scalar_test_empty(s.key);
        }
        {
            SCOPED_TRACE("here 3");
            NodeInit s(sarr);
            node_scalar_test_foo3(s.val);
            node_scalar_test_empty(s.key);
        }

        for(auto s : {
            NodeInit(sarr),
            NodeInit(to_csubstr(sptr)),
            NodeInit(csubstr{sptr, sptrlen}),
            NodeInit(ssp)})
        {
            SCOPED_TRACE("here LOOP");
            node_scalar_test_foo3(s.val);
            node_scalar_test_empty(s.key);
        }
    }
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

TEST(NodeRef, 0_general)
{
    Tree t;

    NodeRef root(&t);

    //using S = csubstr;
    //using V = NodeScalar;
    using N = NodeInit;

    root = N{MAP};
    root.append_child({"a", "0"});
    root.append_child({MAP, "b"});
    root["b"].append_child({SEQ, "seq"});
    root["b"]["seq"].append_child({"0"});
    root["b"]["seq"].append_child({"1"});
    root["b"]["seq"].append_child({"2"});
    root["b"]["seq"].append_child({NodeScalar{"!!str", "3"}});
    auto ch4 = root["b"]["seq"][3].append_sibling({"4"});
    EXPECT_EQ(ch4.id(), root["b"]["seq"][4].id());
    EXPECT_EQ(ch4.get(), root["b"]["seq"][4].get());
    EXPECT_EQ((type_bits)root["b"]["seq"][4].type(), (type_bits)VAL);
    EXPECT_EQ(root["b"]["seq"][4].val(), "4");
    root["b"]["seq"].append_sibling({NodeScalar{"!!str", "aaa"}, NodeScalar{"!!int", "0"}});
    EXPECT_EQ((type_bits)root["b"]["seq"][4].type(), (type_bits)VAL);
    EXPECT_EQ(root["b"]["seq"][4].val(), "4");

    root["b"]["key"] = "val";
    auto seq = root["b"]["seq"];
    auto seq2 = root["b"]["seq2"];
    EXPECT_TRUE(seq2.is_seed());
    root["b"]["seq2"] = N(SEQ);
    seq2 = root["b"]["seq2"];
    EXPECT_FALSE(seq2.is_seed());
    EXPECT_TRUE(seq2.is_seq());
    EXPECT_EQ(seq2.num_children(), 0);
    EXPECT_EQ(root["b"]["seq2"].get(), seq2.get());
    auto seq20 = seq2[0];
    EXPECT_TRUE(seq20.is_seed());
    EXPECT_TRUE(seq2[0].is_seed());
    EXPECT_EQ(seq2.num_children(), 0);
    EXPECT_TRUE(seq2[0].is_seed());
    EXPECT_TRUE(seq20.is_seed());
    EXPECT_NE(seq.get(), seq2.get());
    seq20 = root["b"]["seq2"][0];
    EXPECT_TRUE(seq20.is_seed());
    root["b"]["seq2"][0] = "00";
    seq20 = root["b"]["seq2"][0];
    EXPECT_FALSE(seq20.is_seed());
    NodeRef before = root["b"]["key"];
    EXPECT_EQ(before.key(), "key");
    EXPECT_EQ(before.val(), "val");
    root["b"]["seq2"][1] = "01";
    NodeRef after = root["b"]["key"];
    EXPECT_EQ(before.key(), "key");
    EXPECT_EQ(before.val(), "val");
    EXPECT_EQ(after.key(), "key");
    EXPECT_EQ(after.val(), "val");
    root["b"]["seq2"][2] = "02";
    root["b"]["seq2"][3] = "03";
    int iv = 0;
    root["b"]["seq2"][4] << 55; root["b"]["seq2"][4] >> iv;
    size_t zv = 0;
    root["b"]["seq2"][5] << size_t(55); root["b"]["seq2"][5] >> zv;
    float fv = 0;
    root["b"]["seq2"][6] << 2.0f; root["b"]["seq2"][6] >> fv;
    float dv = 0;
    root["b"]["seq2"][7] << 2.0; root["b"]["seq2"][7] >> dv;

    EXPECT_EQ(root["b"]["key"].key(), "key");
    EXPECT_EQ(root["b"]["key"].val(), "val");


    emit(t);

    EXPECT_EQ((type_bits)root.type(), (type_bits)MAP);
    EXPECT_EQ((type_bits)root["a"].type(), (type_bits)KEYVAL);
    EXPECT_EQ(root["a"].key(), "a");
    EXPECT_EQ(root["a"].val(), "0");

    EXPECT_EQ((type_bits)root["b"].type(), (type_bits)KEYMAP);

    EXPECT_EQ((type_bits)root["b"]["seq"].type(), (type_bits)KEYSEQ);
    EXPECT_EQ(           root["b"]["seq"].key(), "seq");
    EXPECT_EQ((type_bits)root["b"]["seq"][0].type(), (type_bits)VAL);
    EXPECT_EQ(           root["b"]["seq"][0].val(), "0");
    EXPECT_EQ((type_bits)root["b"]["seq"][1].type(), (type_bits)VAL);
    EXPECT_EQ(           root["b"]["seq"][1].val(), "1");
    EXPECT_EQ((type_bits)root["b"]["seq"][2].type(), (type_bits)VAL);
    EXPECT_EQ(           root["b"]["seq"][2].val(), "2");
    EXPECT_EQ((type_bits)root["b"]["seq"][3].type(), (type_bits)VAL);
    EXPECT_EQ(           root["b"]["seq"][3].val(), "3");
    EXPECT_EQ(           root["b"]["seq"][3].val_tag(), "!!str");
    EXPECT_EQ((type_bits)root["b"]["seq"][4].type(), (type_bits)VAL);
    EXPECT_EQ(           root["b"]["seq"][4].val(), "4");

    int tv;
    EXPECT_EQ(root["b"]["key"].key(), "key");
    EXPECT_EQ(root["b"]["key"].val(), "val");
    EXPECT_EQ(root["b"]["seq2"][0].val(), "00"); root["b"]["seq2"][0] >> tv; EXPECT_EQ(tv, 0);
    EXPECT_EQ(root["b"]["seq2"][1].val(), "01"); root["b"]["seq2"][1] >> tv; EXPECT_EQ(tv, 1);
    EXPECT_EQ(root["b"]["seq2"][2].val(), "02"); root["b"]["seq2"][2] >> tv; EXPECT_EQ(tv, 2);
    EXPECT_EQ(root["b"]["seq2"][3].val(), "03"); root["b"]["seq2"][3] >> tv; EXPECT_EQ(tv, 3);
    EXPECT_EQ(root["b"]["seq2"][4].val(), "55"); EXPECT_EQ(iv, 55);
    EXPECT_EQ(root["b"]["seq2"][5].val(), "55"); EXPECT_EQ(zv, size_t(55));
    EXPECT_EQ(root["b"]["seq2"][6].val(), "2"); EXPECT_EQ(fv, 2.f);
    EXPECT_EQ(root["b"]["seq2"][6].val(), "2"); EXPECT_EQ(dv, 2.);

    root["b"]["seq"][2].set_val_serialized(22);

    emit(t);

    EXPECT_EQ((type_bits)root["b"]["aaa"].type(), (type_bits)KEYVAL);
    EXPECT_EQ(root["b"]["aaa"].key_tag(), "!!str");
    EXPECT_EQ(root["b"]["aaa"].key(), "aaa");
    EXPECT_EQ(root["b"]["aaa"].val_tag(), "!!int");
    EXPECT_EQ(root["b"]["aaa"].val(), "0");
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void noderef_check_tree(NodeRef const& root)
{
    test_invariants(*root.tree());

    EXPECT_EQ(root.tree()->size(), 7u);
    EXPECT_EQ(root.num_children(), 6u);
    EXPECT_EQ(root.is_container(), true);
    EXPECT_EQ(root.is_seq(), true);

    EXPECT_EQ((type_bits)root[0].type(), (type_bits)VAL);
    EXPECT_EQ(           root[0].val(), "0");
    EXPECT_EQ((type_bits)root[1].type(), (type_bits)VAL);
    EXPECT_EQ(           root[1].val(), "1");
    EXPECT_EQ((type_bits)root[2].type(), (type_bits)VAL);
    EXPECT_EQ(           root[2].val(), "2");
    EXPECT_EQ((type_bits)root[3].type(), (type_bits)VAL);
    EXPECT_EQ(           root[3].val(), "3");
    EXPECT_EQ((type_bits)root[4].type(), (type_bits)VAL);
    EXPECT_EQ(           root[4].val(), "4");
    EXPECT_EQ((type_bits)root[5].type(), (type_bits)VAL);
    EXPECT_EQ(           root[5].val(), "5");
}

TEST(NodeRef, 1_append_child)
{
    Tree t;

    NodeRef root(&t);

    root |= SEQ;
    root.append_child({"0"});
    root.append_child({"1"});
    root.append_child({"2"});
    root.append_child({"3"});
    root.append_child({"4"});
    root.append_child({"5"});

    noderef_check_tree(root);
}

TEST(NodeRef, 2_prepend_child)
{
    Tree t;

    NodeRef root(&t);

    root |= SEQ;
    root.prepend_child({"5"});
    root.prepend_child({"4"});
    root.prepend_child({"3"});
    root.prepend_child({"2"});
    root.prepend_child({"1"});
    root.prepend_child({"0"});

    noderef_check_tree(root);
}

TEST(NodeRef, 3_insert_child)
{
    Tree t;

    NodeRef root(&t);
    NodeRef none(&t, NONE);

    root |= SEQ;
    root.insert_child({"3"}, none);
    root.insert_child({"4"}, root[0]);
    root.insert_child({"0"}, none);
    root.insert_child({"5"}, root[2]);
    root.insert_child({"1"}, root[0]);
    root.insert_child({"2"}, root[1]);

    noderef_check_tree(root);
}

TEST(NodeRef, 4_remove_child)
{
    Tree t;

    NodeRef root(&t);
    NodeRef none(&t, NONE);

    root |= SEQ;
    root.insert_child({"3"}, none);
    root.insert_child({"4"}, root[0]);
    root.insert_child({"0"}, none);
    root.insert_child({"5"}, root[2]);
    root.insert_child({"1"}, root[0]);
    root.insert_child({"2"}, root[1]);

    std::vector<int> vec({10, 20, 30, 40, 50, 60, 70, 80, 90});
    root.insert_child(root[0]) << vec; // 1
    root.insert_child(root[2]) << vec; // 3
    root.insert_child(root[4]) << vec; // 5
    root.insert_child(root[6]) << vec; // 7
    root.insert_child(root[8]) << vec; // 9
    root.append_child() << vec;        // 10

    root.remove_child(11);
    root.remove_child(9);
    root.remove_child(7);
    root.remove_child(5);
    root.remove_child(3);
    root.remove_child(1);

    noderef_check_tree(root);

    std::vector<std::vector<int>> vec2({{100, 200}, {300, 400}, {500, 600}, {700, 800, 900}});
    root.prepend_child() << vec2; // 0
    root.insert_child(root[1]) << vec2; // 2
    root.insert_child(root[3]) << vec2; // 4
    root.insert_child(root[5]) << vec2; // 6
    root.insert_child(root[7]) << vec2; // 8
    root.insert_child(root[9]) << vec2; // 10
    root.append_child() << vec2;        // 12

    root.remove_child(12);
    root.remove_child(10);
    root.remove_child(8);
    root.remove_child(6);
    root.remove_child(4);
    root.remove_child(2);
    root.remove_child(0);

    noderef_check_tree(root);
}

TEST(NodeRef, 5_move_in_same_parent)
{
    Tree t;
    NodeRef r = t;

    std::vector<std::vector<int>> vec2({{100, 200}, {300, 400}, {500, 600}, {700, 800, 900}});
    std::map<std::string, int> map2({{"foo", 100}, {"bar", 200}, {"baz", 300}});

    r |= SEQ;
    r.append_child() << vec2;
    r.append_child() << map2;
    r.append_child() << "elm2";
    r.append_child() << "elm3";

    auto s = r[0];
    auto m = r[1];
    EXPECT_TRUE(s.is_seq());
    EXPECT_TRUE(m.is_map());
    EXPECT_EQ(s.num_children(), vec2.size());
    EXPECT_EQ(m.num_children(), map2.size());
    //printf("fonix"); print_tree(t); emit(r);
    r[0].move(r[1]);
    //printf("fonix"); print_tree(t); emit(r);
    EXPECT_EQ(r[0].get(), m.get());
    EXPECT_EQ(r[0].num_children(), map2.size());
    EXPECT_EQ(r[1].get(), s.get());
    EXPECT_EQ(r[1].num_children(), vec2.size());
}

TEST(NodeRef, 6_move_to_other_parent)
{
    Tree t;
    NodeRef r = t;

    std::vector<std::vector<int>> vec2({{100, 200}, {300, 400}, {500, 600}, {700, 800, 900}});
    std::map<std::string, int> map2({{"foo", 100}, {"bar", 200}, {"baz", 300}});

    r |= SEQ;
    r.append_child() << vec2;
    r.append_child() << map2;
    r.append_child() << "elm2";
    r.append_child() << "elm3";

    NodeData *elm2 = r[2].get();
    EXPECT_EQ(r[2].val(), "elm2");
    //printf("fonix"); print_tree(t); emit(r);
    r[2].move(r[0], r[0][0]);
    EXPECT_EQ(r[0][1].get(), elm2);
    EXPECT_EQ(r[0][1].val(), "elm2");
    //printf("fonix"); print_tree(t); emit(r);
}

TEST(NodeRef, 7_duplicate)
{
    Tree t;
    NodeRef r = t;

    std::vector<std::vector<int>> vec2({{100, 200}, {300, 400}, {500, 600}, {700, 800, 900}});
    std::map<std::string, int> map2({{"bar", 200}, {"baz", 300}, {"foo", 100}});

    r |= SEQ;
    r.append_child() << vec2;
    r.append_child() << map2;
    r.append_child() << "elm2";
    r.append_child() << "elm3";

    EXPECT_EQ(r[0][0].num_children(), 2u);
    NodeRef dup = r[1].duplicate(r[0][0], r[0][0][1]);
    EXPECT_EQ(r[0][0].num_children(), 3u);
    EXPECT_EQ(r[0][0][2].num_children(), map2.size());
    EXPECT_NE(dup.get(), r[1].get());
    EXPECT_EQ(dup[0].key(), "bar");
    EXPECT_EQ(dup[0].val(), "200");
    EXPECT_EQ(dup[1].key(), "baz");
    EXPECT_EQ(dup[1].val(), "300");
    EXPECT_EQ(dup[2].key(), "foo");
    EXPECT_EQ(dup[2].val(), "100");
    EXPECT_EQ(dup[0].key().str, r[1][0].key().str);
    EXPECT_EQ(dup[0].val().str, r[1][0].val().str);
    EXPECT_EQ(dup[0].key().len, r[1][0].key().len);
    EXPECT_EQ(dup[0].val().len, r[1][0].val().len);
    EXPECT_EQ(dup[1].key().str, r[1][1].key().str);
    EXPECT_EQ(dup[1].val().str, r[1][1].val().str);
    EXPECT_EQ(dup[1].key().len, r[1][1].key().len);
    EXPECT_EQ(dup[1].val().len, r[1][1].val().len);
}

TEST(NodeRef, intseq)
{
    Tree t = parse("iseq: [8, 10]");
    NodeRef n = t["iseq"];
    int a, b;
    n[0] >> a;
    n[1] >> b;
    EXPECT_EQ(a, 8);
    EXPECT_EQ(b, 10);
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
TEST(general, parsing)
{
    auto tree = parse("{foo: 1}");

    char cmpbuf[128] = {0};
    substr cmp(cmpbuf);
    size_t ret;

    ret = cat(cmp, tree["foo"].val());
    EXPECT_EQ(cmp.first(ret), "1");

    ret = cat(cmp, tree["foo"].key());
    EXPECT_EQ(cmp.first(ret), "foo");
}

TEST(general, emitting)
{
    std::string cmpbuf;

    Tree tree;
    auto r = tree.rootref();

    r |= MAP;  // this is needed to make the root a map

    r["foo"] = "1"; // ryml works only with strings.
    // Note that the tree will be __pointing__ at the
    // strings "foo" and "1" used here. You need
    // to make sure they have at least the same
    // lifetime as the tree.

    auto s = r["seq"]; // does not change the tree until s is written to.
    s |= SEQ;
    r["seq"].append_child() = "bar0"; // value of this child is now __pointing__ at "bar0"
    r["seq"].append_child() = "bar1";
    r["seq"].append_child() = "bar2";

    //print_tree(tree);

    // emit to stdout (can also emit to FILE* or ryml::span)
    emitrs(tree, &cmpbuf);
    const char* exp = R"(foo: 1
seq:
  - bar0
  - bar1
  - bar2
)";
    EXPECT_EQ(cmpbuf, exp);

    // serializing: using operator<< instead of operator=
    // will make the tree serialize the value into a char
    // arena inside the tree. This arena can be reserved at will.
    int ch3 = 33, ch4 = 44;
    s.append_child() << ch3;
    s.append_child() << ch4;

    {
        std::string tmp = "child5";
        s.append_child() << tmp;   // requires #include <c4/yml/std/string.hpp>
        // now tmp can go safely out of scope, as it was
        // serialized to the tree's internal string arena
        // Note the include highlighted above is required so that ryml
        // knows how to turn an std::string into a c4::csubstr/c4::substr.
    }

    emitrs(tree, &cmpbuf);
    exp = R"(foo: 1
seq:
  - bar0
  - bar1
  - bar2
  - 33
  - 44
  - child5
)";
    EXPECT_EQ(cmpbuf, exp);

    // to serialize keys:
    int k=66;
    r.append_child() << key(k) << 7;

    emitrs(tree, &cmpbuf);
    exp = R"(foo: 1
seq:
  - bar0
  - bar1
  - bar2
  - 33
  - 44
  - child5
66: 7
)";
    EXPECT_EQ(cmpbuf, exp);
}

TEST(general, map_to_root)
{
    std::string cmpbuf; const char *exp;
    std::map<std::string, int> m({{"bar", 2}, {"foo", 1}});
    Tree t;
    t.rootref() << m;

    emitrs(t, &cmpbuf);
    exp = R"(bar: 2
foo: 1
)";
    EXPECT_EQ(cmpbuf, exp);

    t["foo"] << 10;
    t["bar"] << 20;

    m.clear();
    t.rootref() >> m;

    EXPECT_EQ(m["foo"], 10);
    EXPECT_EQ(m["bar"], 20);
}

TEST(general, print_tree)
{
    const char yaml[] = R"(
a:
  b: bval
  c:
    d:
      - e
      - d
      - f: fval
        g: gval
        h:
          -
            x: a
            y: b
          -
            z: c
            u:
)";
    Tree t = parse(yaml);
    print_tree(t); // to make sure this is covered too
}

TEST(general, numbers)
{
    const char yaml[] = R"(- -1
- -1.0
- +1.0
- 1e-2
- 1e+2
)";
    Tree t = parse(yaml);
    auto s = emitrs<std::string>(t);
    EXPECT_EQ(s, std::string(yaml));
}

// github issue 29: https://github.com/biojppm/rapidyaml/issues/29
TEST(general, newlines_on_maps_nested_in_seqs)
{
    const char yaml[] = R"(enemy:
- actors:
  - {name: Enemy_Bokoblin_Junior, value: 4.0}
  - {name: Enemy_Bokoblin_Middle, value: 16.0}
  - {name: Enemy_Bokoblin_Senior, value: 32.0}
  - {name: Enemy_Bokoblin_Dark, value: 48.0}
  species: BokoblinSeries
)";
    std::string expected = R"(enemy:
  - actors:
      - name: Enemy_Bokoblin_Junior
        value: 4.0
      - name: Enemy_Bokoblin_Middle
        value: 16.0
      - name: Enemy_Bokoblin_Senior
        value: 32.0
      - name: Enemy_Bokoblin_Dark
        value: 48.0
    species: BokoblinSeries
)";
    Tree t = parse(yaml);
    auto s = emitrs<std::string>(t);
    EXPECT_EQ(expected, s);
}

TEST(general, lookup_path)
{
    const char yaml[] = R"(
a:
  b: bval
  c:
    d:
      - e
      - d
      - f: fval
        g: gval
        h:
          -
            x: a
            y: b
          -
            z: c
            u:
)";
    Tree t = parse(yaml);
    print_tree(t);

    EXPECT_EQ(t.lookup_path("a").target, 1);
    EXPECT_EQ(t.lookup_path("a.b").target, 2);
    EXPECT_EQ(t.lookup_path("a.c").target, 3);
    EXPECT_EQ(t.lookup_path("a.c.d").target, 4);
    EXPECT_EQ(t.lookup_path("a.c.d[0]").target, 5);
    EXPECT_EQ(t.lookup_path("a.c.d[1]").target, 6);
    EXPECT_EQ(t.lookup_path("a.c.d[2]").target, 7);
    EXPECT_EQ(t.lookup_path("a.c.d[2].f").target, 8);
    EXPECT_EQ(t.lookup_path("a.c.d[2].g").target, 9);
    EXPECT_EQ(t.lookup_path("a.c.d[2].h").target, 10);
    EXPECT_EQ(t.lookup_path("a.c.d[2].h[0]").target, 11);
    EXPECT_EQ(t.lookup_path("a.c.d[2].h[0].x").target, 12);
    EXPECT_EQ(t.lookup_path("a.c.d[2].h[0].y").target, 13);
    EXPECT_EQ(t.lookup_path("a.c.d[2].h[1]").target, 14);
    EXPECT_EQ(t.lookup_path("a.c.d[2].h[1].z").target, 15);
    EXPECT_EQ(t.lookup_path("a.c.d[2].h[1].u").target, 16);
    EXPECT_EQ(t.lookup_path("d", 3).target, 4);
    EXPECT_EQ(t.lookup_path("d[0]", 3).target, 5);
    EXPECT_EQ(t.lookup_path("d[1]", 3).target, 6);
    EXPECT_EQ(t.lookup_path("d[2]", 3).target, 7);
    EXPECT_EQ(t.lookup_path("d[2].f", 3).target, 8);
    EXPECT_EQ(t.lookup_path("d[2].g", 3).target, 9);
    EXPECT_EQ(t.lookup_path("d[2].h", 3).target, 10);
    EXPECT_EQ(t.lookup_path("d[2].h[0]", 3).target, 11);
    EXPECT_EQ(t.lookup_path("d[2].h[0].x", 3).target, 12);
    EXPECT_EQ(t.lookup_path("d[2].h[0].y", 3).target, 13);
    EXPECT_EQ(t.lookup_path("d[2].h[1]", 3).target, 14);
    EXPECT_EQ(t.lookup_path("d[2].h[1].z", 3).target, 15);
    EXPECT_EQ(t.lookup_path("d[2].h[1].u", 3).target, 16);

    auto lp = t.lookup_path("x");
    EXPECT_FALSE(lp);
    EXPECT_EQ(lp.target, (size_t)NONE);
    EXPECT_EQ(lp.closest, (size_t)NONE);
    EXPECT_EQ(lp.resolved(), "");
    EXPECT_EQ(lp.unresolved(), "x");
    lp = t.lookup_path("a.x");
    EXPECT_FALSE(lp);
    EXPECT_EQ(lp.target, (size_t)NONE);
    EXPECT_EQ(lp.closest, 1);
    EXPECT_EQ(lp.resolved(), "a");
    EXPECT_EQ(lp.unresolved(), "x");
    lp = t.lookup_path("a.b.x");
    EXPECT_FALSE(lp);
    EXPECT_EQ(lp.target, (size_t)NONE);
    EXPECT_EQ(lp.closest, 2);
    EXPECT_EQ(lp.resolved(), "a.b");
    EXPECT_EQ(lp.unresolved(), "x");
    lp = t.lookup_path("a.c.x");
    EXPECT_FALSE(lp);
    EXPECT_EQ(lp.target, (size_t)NONE);
    EXPECT_EQ(lp.closest, 3);
    EXPECT_EQ(lp.resolved(), "a.c");
    EXPECT_EQ(lp.unresolved(), "x");

    size_t sz = t.size();
    EXPECT_EQ(t.lookup_path("x").target, (size_t)NONE);
    EXPECT_EQ(t.lookup_path_or_modify("x", "x"), sz);
    EXPECT_EQ(t.lookup_path("x").target, sz);
    EXPECT_EQ(t.val(sz), "x");
    EXPECT_EQ(t.lookup_path_or_modify("y", "x"), sz);
    EXPECT_EQ(t.val(sz), "y");
    EXPECT_EQ(t.lookup_path_or_modify("z", "x"), sz);
    EXPECT_EQ(t.val(sz), "z");

    sz = t.size();
    EXPECT_EQ(t.lookup_path("a.x").target, (size_t)NONE);
    EXPECT_EQ(t.lookup_path_or_modify("x", "a.x"), sz);
    EXPECT_EQ(t.lookup_path("a.x").target, sz);
    EXPECT_EQ(t.val(sz), "x");
    EXPECT_EQ(t.lookup_path_or_modify("y", "a.x"), sz);
    EXPECT_EQ(t.val(sz), "y");
    EXPECT_EQ(t.lookup_path_or_modify("z", "a.x"), sz);
    EXPECT_EQ(t.val(sz), "z");

    sz = t.size();
    EXPECT_EQ(t.lookup_path("a.c.x").target, (size_t)NONE);
    EXPECT_EQ(t.lookup_path_or_modify("x", "a.c.x"), sz);
    EXPECT_EQ(t.lookup_path("a.c.x").target, sz);
    EXPECT_EQ(t.val(sz), "x");
    EXPECT_EQ(t.lookup_path_or_modify("y", "a.c.x"), sz);
    EXPECT_EQ(t.val(sz), "y");
    EXPECT_EQ(t.lookup_path_or_modify("z", "a.c.x"), sz);
    EXPECT_EQ(t.val(sz), "z");
}

TEST(general, lookup_path_or_modify)
{
    {
        Tree dst = parse("{}");
        Tree const src = parse("{d: [x, y, z]}");
        dst.lookup_path_or_modify("ok", "a.b.c");
        EXPECT_EQ(dst["a"]["b"]["c"].val(), "ok");
        dst.lookup_path_or_modify(&src, src["d"].id(), "a.b.d");
        EXPECT_EQ(dst["a"]["b"]["d"][0].val(), "x");
        EXPECT_EQ(dst["a"]["b"]["d"][1].val(), "y");
        EXPECT_EQ(dst["a"]["b"]["d"][2].val(), "z");
    }

    {
        Tree t = parse("{}");
        csubstr bigpath = "newmap.newseq[0].newmap.newseq[0].first";
        auto result = t.lookup_path(bigpath);
        EXPECT_EQ(result.target, (size_t)NONE);
        EXPECT_EQ(result.closest, (size_t)NONE);
        EXPECT_EQ(result.resolved(), "");
        EXPECT_EQ(result.unresolved(), bigpath);
        size_t sz = t.lookup_path_or_modify("x", bigpath);
        EXPECT_EQ(t.lookup_path(bigpath).target, sz);
        EXPECT_EQ(t.val(sz), "x");
        EXPECT_EQ(t["newmap"]["newseq"].num_children(), 1u);
        EXPECT_EQ(t["newmap"]["newseq"][0].is_map(), true);
        EXPECT_EQ(t["newmap"]["newseq"][0]["newmap"].is_map(), true);
        EXPECT_EQ(t["newmap"]["newseq"][0]["newmap"]["newseq"].is_seq(), true);
        EXPECT_EQ(t["newmap"]["newseq"][0]["newmap"]["newseq"].num_children(), 1u);
        EXPECT_EQ(t["newmap"]["newseq"][0]["newmap"]["newseq"][0].is_map(), true);
        EXPECT_EQ(t["newmap"]["newseq"][0]["newmap"]["newseq"][0]["first"].val(), "x");
        size_t sz2 = t.lookup_path_or_modify("y", bigpath);
        EXPECT_EQ(t["newmap"]["newseq"][0]["newmap"]["newseq"][0]["first"].val(), "y");
        EXPECT_EQ(sz2, sz);
        EXPECT_EQ(t.lookup_path(bigpath).target, sz);
        EXPECT_EQ(t.val(sz2), "y");

        sz2 = t.lookup_path_or_modify("y", "newmap2.newseq2[2].newmap2.newseq2[3].first2");
        EXPECT_EQ(t.lookup_path("newmap2.newseq2[2].newmap2.newseq2[3].first2").target, sz2);
        EXPECT_EQ(t.val(sz2), "y");
        EXPECT_EQ(t["newmap2"]["newseq2"].num_children(), 3u);
        EXPECT_EQ(t["newmap2"]["newseq2"][0].val(), nullptr);
        EXPECT_EQ(t["newmap2"]["newseq2"][1].val(), nullptr);
        EXPECT_EQ(t["newmap2"]["newseq2"][2].is_map(), true);
        EXPECT_EQ(t["newmap2"]["newseq2"][2]["newmap2"].is_map(), true);
        EXPECT_EQ(t["newmap2"]["newseq2"][2]["newmap2"]["newseq2"].is_seq(), true);
        EXPECT_EQ(t["newmap2"]["newseq2"][2]["newmap2"]["newseq2"].num_children(), 4u);
        EXPECT_EQ(t["newmap2"]["newseq2"][2]["newmap2"]["newseq2"][0].val(), nullptr);
        EXPECT_EQ(t["newmap2"]["newseq2"][2]["newmap2"]["newseq2"][1].val(), nullptr);
        EXPECT_EQ(t["newmap2"]["newseq2"][2]["newmap2"]["newseq2"][2].val(), nullptr);
        EXPECT_EQ(t["newmap2"]["newseq2"][2]["newmap2"]["newseq2"][3].is_map(), true);
        EXPECT_EQ(t["newmap2"]["newseq2"][2]["newmap2"]["newseq2"][3]["first2"].val(), "y");
        sz2 = t.lookup_path_or_modify("z", "newmap2.newseq2[2].newmap2.newseq2[3].second2");
        EXPECT_EQ  (t["newmap2"]["newseq2"][2]["newmap2"]["newseq2"][3]["second2"].val(), "z");

        sz = t.lookup_path_or_modify("foo", "newmap.newseq1[1]");
        EXPECT_EQ(t["newmap"].is_map(), true);
        EXPECT_EQ(t["newmap"]["newseq1"].is_seq(), true);
        EXPECT_EQ(t["newmap"]["newseq1"].num_children(), 2u);
        EXPECT_EQ(t["newmap"]["newseq1"][0].val(), nullptr);
        EXPECT_EQ(t["newmap"]["newseq1"][1].val(), "foo");
        sz = t.lookup_path_or_modify("bar", "newmap.newseq1[2][1]");
        EXPECT_EQ(t["newmap"]["newseq1"].is_seq(), true);
        EXPECT_EQ(t["newmap"]["newseq1"].num_children(), 3u);
        EXPECT_EQ(t["newmap"]["newseq1"][0].val(), nullptr);
        EXPECT_EQ(t["newmap"]["newseq1"][1].val(), "foo");
        EXPECT_EQ(t["newmap"]["newseq1"][2].is_seq(), true);
        EXPECT_EQ(t["newmap"]["newseq1"][2].num_children(), 2u);
        EXPECT_EQ(t["newmap"]["newseq1"][2][0].val(), nullptr);
        EXPECT_EQ(t["newmap"]["newseq1"][2][1].val(), "bar");
        sz = t.lookup_path_or_modify("Foo?"   , "newmap.newseq1[0]");
        sz = t.lookup_path_or_modify("Bar?"   , "newmap.newseq1[2][0]");
        sz = t.lookup_path_or_modify("happy"  , "newmap.newseq1[2][2][3]");
        sz = t.lookup_path_or_modify("trigger", "newmap.newseq1[2][2][2]");
        sz = t.lookup_path_or_modify("Arnold" , "newmap.newseq1[2][2][0]");
        sz = t.lookup_path_or_modify("is"     , "newmap.newseq1[2][2][1]");
        EXPECT_EQ(t["newmap"]["newseq1"].is_seq(), true);
        EXPECT_EQ(t["newmap"]["newseq1"].num_children(), 3u);
        EXPECT_EQ(t["newmap"]["newseq1"][0].val(), "Foo?");
        EXPECT_EQ(t["newmap"]["newseq1"][1].val(), "foo");
        EXPECT_EQ(t["newmap"]["newseq1"][2].is_seq(), true);
        EXPECT_EQ(t["newmap"]["newseq1"][2].num_children(), 3u);
        EXPECT_EQ(t["newmap"]["newseq1"][2][0].val(), "Bar?");
        EXPECT_EQ(t["newmap"]["newseq1"][2][1].val(), "bar");
        EXPECT_EQ(t["newmap"]["newseq1"][2][2].is_seq(), true);
        EXPECT_EQ(t["newmap"]["newseq1"][2][2].num_children(), 4u);
        EXPECT_EQ(t["newmap"]["newseq1"][2][2][0].val(), "Arnold");
        EXPECT_EQ(t["newmap"]["newseq1"][2][2][1].val(), "is");
        EXPECT_EQ(t["newmap"]["newseq1"][2][2][2].val(), "trigger");
        EXPECT_EQ(t["newmap"]["newseq1"][2][2][3].val(), "happy");

        EXPECT_EQ(emitrs<std::string>(t), R"(newmap:
  newseq:
    - newmap:
        newseq:
          - first: y
  newseq1:
    - 'Foo?'
    - foo
    - - 'Bar?'
      - bar
      - - Arnold
        - is
        - trigger
        - happy
newmap2:
  newseq2:
    - ~
    - ~
    - newmap2:
        newseq2:
          - ~
          - ~
          - ~
          - first2: y
            second2: z
)");
    }
}


//-------------------------------------------

TEST(general, github_issue_124)
{
    // All these inputs are basically the same.
    // However, the comment was found to confuse the parser in #124.
    csubstr yaml[] = {
        "a:\n  - b\nc: d",
        "a:\n  - b\n\n# ignore me:\nc: d",
        "a:\n  - b\n\n  # ignore me:\nc: d",
        "a:\n  - b\n\n    # ignore me:\nc: d",
        "a:\n  - b\n\n#:\nc: d", // also try with just a ':' in the comment
        "a:\n  - b\n\n# :\nc: d",
        "a:\n  - b\n\n#\nc: d",  // also try with empty comment
    };
    for(csubstr inp : yaml)
    {
        SCOPED_TRACE(inp);
        Tree t = parse(inp);
        std::string s = emitrs<std::string>(t);
        // The re-emitted output should not contain the comment.
        EXPECT_EQ(c4::to_csubstr(s), "a:\n  - b\nc: d\n");
    }
}


//-----------------------------------------------------------------------------

TEST(set_root_as_stream, empty_tree)
{
    Tree t;
    NodeRef r = t.rootref();
    EXPECT_EQ(r.is_stream(), false);
    EXPECT_EQ(r.num_children(), 0u);
    t.set_root_as_stream();
    EXPECT_EQ(r.is_stream(), true);
    EXPECT_EQ(r.num_children(), 0u);
}

TEST(set_root_as_stream, already_with_stream)
{
    Tree t;
    t.to_stream(t.root_id());
    NodeRef r = t.rootref();
    EXPECT_EQ(r.is_stream(), true);
    EXPECT_EQ(r.num_children(), 0u);
    t.set_root_as_stream();
    EXPECT_EQ(r.is_stream(), true);
    EXPECT_EQ(r.num_children(), 0u);
}


TEST(set_root_as_stream, root_is_map)
{
    Tree t = parse(R"({a: b, c: d})");
    NodeRef r = t.rootref();
    EXPECT_EQ(r.is_stream(), false);
    EXPECT_EQ(r.is_doc(), false);
    EXPECT_EQ(r.is_map(), true);
    EXPECT_EQ(r.is_seq(), false);
    EXPECT_EQ(r.num_children(), 2u);
    print_tree(t);
    std::cout << t;
    t.set_root_as_stream();
    print_tree(t);
    std::cout << t;
    EXPECT_EQ(r.is_stream(), true);
    EXPECT_EQ(r.is_doc(), false);
    EXPECT_EQ(r.is_map(), false);
    EXPECT_EQ(r.is_seq(), true);
    EXPECT_EQ(r.num_children(), 1u);
    EXPECT_EQ(r[0].is_doc(), true);
    EXPECT_EQ(r[0].is_map(), true);
    EXPECT_EQ(r[0].is_seq(), false);
    EXPECT_EQ(r[0].num_children(), 2u);
    EXPECT_EQ(r[0]["a"].is_keyval(), true);
    EXPECT_EQ(r[0]["c"].is_keyval(), true);
    EXPECT_EQ(r[0]["a"].val(), "b");
    EXPECT_EQ(r[0]["c"].val(), "d");
}

TEST(set_root_as_stream, root_is_docmap)
{
    Tree t = parse(R"({a: b, c: d})");
    t._p(t.root_id())->m_type.add(DOC);
    NodeRef r = t.rootref();
    EXPECT_EQ(r.is_stream(), false);
    EXPECT_EQ(r.is_doc(), true);
    EXPECT_EQ(r.is_map(), true);
    EXPECT_EQ(r.is_seq(), false);
    EXPECT_EQ(r.num_children(), 2u);
    print_tree(t);
    std::cout << t;
    t.set_root_as_stream();
    print_tree(t);
    std::cout << t;
    EXPECT_EQ(r.is_stream(), true);
    EXPECT_EQ(r.is_doc(), false);
    EXPECT_EQ(r.is_map(), false);
    EXPECT_EQ(r.is_seq(), true);
    EXPECT_EQ(r.num_children(), 1u);
    EXPECT_EQ(r[0].is_doc(), true);
    EXPECT_EQ(r[0].is_map(), true);
    EXPECT_EQ(r[0].is_seq(), false);
    EXPECT_EQ(r[0].num_children(), 2u);
    EXPECT_EQ(r[0]["a"].is_keyval(), true);
    EXPECT_EQ(r[0]["c"].is_keyval(), true);
    EXPECT_EQ(r[0]["a"].val(), "b");
    EXPECT_EQ(r[0]["c"].val(), "d");
}


TEST(set_root_as_stream, root_is_seq)
{
    Tree t = parse(R"([a, b, c, d])");
    NodeRef r = t.rootref();
    EXPECT_EQ(r.is_stream(), false);
    EXPECT_EQ(r.is_doc(), false);
    EXPECT_EQ(r.is_map(), false);
    EXPECT_EQ(r.is_seq(), true);
    EXPECT_EQ(r.num_children(), 4u);
    print_tree(t);
    std::cout << t;
    t.set_root_as_stream();
    print_tree(t);
    std::cout << t;
    EXPECT_EQ(r.is_stream(), true);
    EXPECT_EQ(r.is_doc(), false);
    EXPECT_EQ(r.is_map(), false);
    EXPECT_EQ(r.is_seq(), true);
    EXPECT_EQ(r.num_children(), 1u);
    EXPECT_EQ(r[0].is_doc(), true);
    EXPECT_EQ(r[0].is_map(), false);
    EXPECT_EQ(r[0].is_seq(), true);
    EXPECT_EQ(r[0].num_children(), 4u);
    EXPECT_EQ(r[0][0].val(), "a");
    EXPECT_EQ(r[0][1].val(), "b");
    EXPECT_EQ(r[0][2].val(), "c");
    EXPECT_EQ(r[0][3].val(), "d");
}

TEST(set_root_as_stream, root_is_docseq)
{
    Tree t = parse(R"([a, b, c, d])");
    t._p(t.root_id())->m_type.add(DOC);
    NodeRef r = t.rootref();
    EXPECT_EQ(r.is_stream(), false);
    EXPECT_EQ(r.is_doc(), true);
    EXPECT_EQ(r.is_map(), false);
    EXPECT_EQ(r.is_seq(), true);
    EXPECT_EQ(r.num_children(), 4u);
    print_tree(t);
    std::cout << t;
    t.set_root_as_stream();
    print_tree(t);
    std::cout << t;
    EXPECT_EQ(r.is_stream(), true);
    EXPECT_EQ(r.is_doc(), false);
    EXPECT_EQ(r.is_map(), false);
    EXPECT_EQ(r.is_seq(), true);
    EXPECT_EQ(r.num_children(), 1u);
    EXPECT_EQ(r[0].is_doc(), true);
    EXPECT_EQ(r[0].is_map(), false);
    EXPECT_EQ(r[0].is_seq(), true);
    EXPECT_EQ(r[0].num_children(), 4u);
    EXPECT_EQ(r[0][0].val(), "a");
    EXPECT_EQ(r[0][1].val(), "b");
    EXPECT_EQ(r[0][2].val(), "c");
    EXPECT_EQ(r[0][3].val(), "d");
}

TEST(set_root_as_stream, root_is_seqmap)
{
    Tree t = parse(R"([{a: b, c: d}, {e: e, f: f}, {g: g, h: h}, {i: i, j: j}])");
    NodeRef r = t.rootref();
    EXPECT_EQ(r.is_stream(), false);
    EXPECT_EQ(r.is_doc(), false);
    EXPECT_EQ(r.is_map(), false);
    EXPECT_EQ(r.is_seq(), true);
    EXPECT_EQ(r.num_children(), 4u);
    print_tree(t);
    std::cout << t;
    t.set_root_as_stream();
    print_tree(t);
    std::cout << t;
    EXPECT_EQ(r.is_stream(), true);
    EXPECT_EQ(r.is_doc(), false);
    EXPECT_EQ(r.is_map(), false);
    EXPECT_EQ(r.is_seq(), true);
    EXPECT_EQ(r.num_children(), 1u);
    EXPECT_EQ(r[0].is_doc(), true);
    EXPECT_EQ(r[0].is_map(), false);
    EXPECT_EQ(r[0].is_seq(), true);
    EXPECT_EQ(r[0].num_children(), 4u);
    EXPECT_EQ(r[0][0].is_map(), true);
    EXPECT_EQ(r[0][1].is_map(), true);
    EXPECT_EQ(r[0][2].is_map(), true);
    EXPECT_EQ(r[0][3].is_map(), true);
    EXPECT_EQ(r[0][0]["a"].val(), "b");
    EXPECT_EQ(r[0][0]["c"].val(), "d");
    EXPECT_EQ(r[0][1]["e"].val(), "e");
    EXPECT_EQ(r[0][1]["f"].val(), "f");
    EXPECT_EQ(r[0][2]["g"].val(), "g");
    EXPECT_EQ(r[0][2]["h"].val(), "h");
    EXPECT_EQ(r[0][3]["i"].val(), "i");
    EXPECT_EQ(r[0][3]["j"].val(), "j");
}

TEST(set_root_as_stream, root_is_mapseq)
{
    Tree t = parse(R"({a: [0, 1, 2], b: [3, 4, 5], c: [6, 7, 8]})");
    NodeRef r = t.rootref();
    EXPECT_EQ(r.is_stream(), false);
    EXPECT_EQ(r.is_doc(), false);
    EXPECT_EQ(r.is_map(), true);
    EXPECT_EQ(r.is_seq(), false);
    EXPECT_EQ(r.num_children(), 3u);
    print_tree(t);
    std::cout << t;
    t.set_root_as_stream();
    print_tree(t);
    std::cout << t;
    EXPECT_EQ(r.is_stream(), true);
    EXPECT_EQ(r.is_doc(), false);
    EXPECT_EQ(r.is_map(), false);
    EXPECT_EQ(r.is_seq(), true);
    EXPECT_EQ(r.num_children(), 1u);
    EXPECT_EQ(r[0].is_doc(), true);
    EXPECT_EQ(r[0].is_map(), true);
    EXPECT_EQ(r[0].is_seq(), false);
    EXPECT_EQ(r[0].num_children(), 3u);
    EXPECT_EQ(r[0]["a"].is_seq(), true);
    EXPECT_EQ(r[0]["b"].is_seq(), true);
    EXPECT_EQ(r[0]["c"].is_seq(), true);
    EXPECT_EQ(r[0]["a"][0].val(), "0");
    EXPECT_EQ(r[0]["a"][1].val(), "1");
    EXPECT_EQ(r[0]["a"][2].val(), "2");
    EXPECT_EQ(r[0]["b"][0].val(), "3");
    EXPECT_EQ(r[0]["b"][1].val(), "4");
    EXPECT_EQ(r[0]["b"][2].val(), "5");
    EXPECT_EQ(r[0]["c"][0].val(), "6");
    EXPECT_EQ(r[0]["c"][1].val(), "7");
    EXPECT_EQ(r[0]["c"][2].val(), "8");
}

TEST(set_root_as_stream, root_is_docval)
{
    Tree t;
    NodeRef r = t.rootref();
    r.set_type(DOCVAL);
    r.set_val("bar");
    r.set_val_tag("<!foo>");
    EXPECT_EQ(r.is_stream(), false);
    EXPECT_EQ(r.is_doc(), true);
    EXPECT_EQ(r.is_map(), false);
    EXPECT_EQ(r.is_seq(), false);
    EXPECT_EQ(r.is_val(), true);
    EXPECT_EQ(r.has_val_tag(), true);
    EXPECT_EQ(r.val_tag(), "<!foo>");
    EXPECT_EQ(r.num_children(), 0u);
    print_tree(t);
    std::cout << t;
    t.set_root_as_stream();
    print_tree(t);
    std::cout << t;
    EXPECT_EQ(r.is_stream(), true);
    EXPECT_EQ(r.is_doc(), false);
    EXPECT_EQ(r.is_map(), false);
    EXPECT_EQ(r.is_seq(), true);
    EXPECT_EQ(r.is_val(), false);
    ASSERT_EQ(r.num_children(), 1u);
    EXPECT_EQ(r[0].is_stream(), false);
    EXPECT_EQ(r[0].is_doc(), true);
    EXPECT_EQ(r[0].is_map(), false);
    EXPECT_EQ(r[0].is_seq(), false);
    EXPECT_EQ(r[0].is_val(), true);
    EXPECT_EQ(r[0].has_val_tag(), true);
    EXPECT_EQ(r[0].val_tag(), "<!foo>");
    EXPECT_EQ(r[0].num_children(), 0u);
}


//-------------------------------------------
// this is needed to use the test case library
Case const* get_case(csubstr /*name*/)
{
    return nullptr;
}

} // namespace yml
} // namespace c4

#if defined(_MSC_VER)
#   pragma warning(pop)
#elif defined(__clang__)
#   pragma clang diagnostic pop
#elif defined(__GNUC__)
#   pragma GCC diagnostic pop
#endif
