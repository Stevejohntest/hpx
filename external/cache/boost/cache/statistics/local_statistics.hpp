//  Copyright (c) 2007-2012 Hartmut Kaiser
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#if !defined(BOOST_CACHE_LOCAL_STATISTICS_NOV_20_2008_1148AM)
#define BOOST_CACHE_LOCAL_STATISTICS_NOV_20_2008_1148AM

///////////////////////////////////////////////////////////////////////////////
namespace boost { namespace cache { namespace statistics
{
    ///////////////////////////////////////////////////////////////////////////
    class local_statistics
    {
    public:
        local_statistics()
          : hits_(0), misses_(0), insertions_(0), evictions_(0)
        {}

        std::size_t hits() const { return hits_; }
        std::size_t misses() const { return misses_; }
        std::size_t insertions() const { return insertions_; }
        std::size_t evictions() const { return evictions_; }

        /// \brief  The function \a got_hit will be called by a cache instance
        ///         whenever a entry got touched.
        void got_hit() { ++hits_; }

        /// \brief  The function \a got_miss will be called by a cache instance
        ///         whenever a requested entry has not been found in the cache.
        void got_miss() { ++ misses_; }

        /// \brief  The function \a got_insertion will be called by a cache
        ///         instance whenever a new entry has been inserted.
        void got_insertion() { ++insertions_; }

        /// \brief  The function \a got_eviction will be called by a cache
        ///         instance whenever an entry has been removed from the cache
        ///         because a new inserted entry let the cache grow beyond its
        ///         capacity.
        void got_eviction() { ++evictions_; }

        /// \brief Reset all statistics
        void clear()
        {
            hits_ = 0;
            misses_ = 0;
            evictions_ = 0;
            insertions_ = 0;
        }

    private:
        std::size_t hits_;
        std::size_t misses_;
        std::size_t insertions_;
        std::size_t evictions_;
    };

}}}

#endif

