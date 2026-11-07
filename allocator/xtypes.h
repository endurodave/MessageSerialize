#ifndef _XTYPES_H
#define _XTYPES_H

/// @file xtypes.h
/// @brief Selects the allocator for the dmq::x* container and string types.
///
/// With DMQ_ALLOCATOR defined (CMake option MSG_SERIALIZE_ALLOCATOR=ON),
/// dmq::xstring, xlist, xmap, xset, xstringstream, ... use the fixed-block
/// allocator (stl_allocator, xallocator), and the XALLOCATOR macro gives a
/// class fixed-block operator new/delete. Without it they are the same std::
/// types on the global heap, and XALLOCATOR is empty. Declare message fields
/// with these types and the build option picks the memory, with no code
/// changes. msg_serialize.h handles both, since it accepts any allocator.

#ifdef DMQ_ALLOCATOR
    #include "xstring.h"
    #include "xlist.h"
    #include "xmap.h"
    #include "xset.h"
    #include "xqueue.h"
    #include "xsstream.h"
    #include "stl_allocator.h"
    #include "xnew.h"
    #include "xmake_shared.h"
#else
    #include <string>
    #include <list>
    #include <map>
    #include <set>
    #include <queue>
    #include <deque>
    #include <sstream>
    #include <memory>
    #include <new>
    #include <utility>

    // Not using xallocator; define as nothing
    #undef XALLOCATOR
    #define XALLOCATOR

    namespace dmq {
        template <typename T>
        using stl_allocator = std::allocator<T>;

        template <typename T, typename Alloc = stl_allocator<T>>
        using xlist = std::list<T, Alloc>;

        typedef std::basic_ostringstream<char, std::char_traits<char>, stl_allocator<char>> xostringstream;
        typedef std::basic_stringstream<char, std::char_traits<char>, stl_allocator<char>> xstringstream;

        typedef std::basic_string<char, std::char_traits<char>, stl_allocator<char>> xstring;

        template <typename T, typename... Args>
        inline std::shared_ptr<T> xmake_shared(Args&&... args)
        {
            return std::make_shared<T>(std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        inline T* xnew(Args&&... args)
        {
            return new(std::nothrow) T(std::forward<Args>(args)...);
        }

        template <typename T>
        inline void xdelete(T* p)
        {
            delete p;
        }

        template <typename Key, typename Value, typename Alloc = stl_allocator<std::pair<const Key, Value>>>
        using xmap = std::map<Key, Value, std::less<Key>, Alloc>;

        template <typename Key, typename Value, typename Alloc = stl_allocator<std::pair<const Key, Value>>>
        using xmultimap = std::multimap<Key, Value, std::less<Key>, Alloc>;

        template <typename Key, typename Compare = std::less<Key>, typename Alloc = stl_allocator<Key>>
        using xset = std::set<Key, Compare, Alloc>;

        template <typename Key, typename Compare = std::less<Key>, typename Alloc = stl_allocator<Key>>
        using xmultiset = std::multiset<Key, Compare, Alloc>;

        template <typename T, typename Alloc = stl_allocator<T>>
        using xqueue = std::queue<T, std::deque<T, Alloc>>;
    }
#endif

#endif // _XTYPES_H
