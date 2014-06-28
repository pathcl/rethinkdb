#ifndef CONTAINERS_ARCHIVE_VERSIONED_HPP_
#define CONTAINERS_ARCHIVE_VERSIONED_HPP_

#include "containers/archive/archive.hpp"
#include "version.hpp"

namespace archive_internal {

// This is just used to implement serialize_cluster_version and
// deserialize_cluster_version.  (cluster_version_t conveniently has a contiguous set
// of valid representation, from v1_13 to v1_13_2_is_latest).
ARCHIVE_PRIM_MAKE_RANGED_SERIALIZABLE(cluster_version_t, int8_t,
                                      cluster_version_t::v1_13,
                                      cluster_version_t::v1_13_2_is_latest);

class bogus_made_up_type_t;

}  // namespace archive_details

// These are generally universal.  They must not have their behavior change -- except
// if we remove some cluster_version_t value, in which case... maybe would fail on a
// range error with the specific removed values.  Or maybe, we would do something
// differently.
inline void serialize_cluster_version(write_message_t *wm, cluster_version_t v) {
    archive_internal::serialize<cluster_version_t::LATEST_OVERALL>(wm, v);
}

inline archive_result_t deserialize_cluster_version(read_stream_t *s,
                                                    cluster_version_t *thing) {
    return archive_internal::deserialize<cluster_version_t::LATEST_OVERALL>(s, thing);
}


// Serializes a value for a given version.  DOES NOT SERIALIZE THE VERSION NUMBER!
template <class T>
void serialize_for_version(cluster_version_t version, write_message_t *wm,
                           const T &value) {
    switch (version) {
    case cluster_version_t::v1_13:
        serialize<cluster_version_t::v1_13>(wm, value);
        break;
    case cluster_version_t::v1_13_2_is_latest:
        serialize<cluster_version_t::v1_13_2_is_latest>(wm, value);
        break;
    default:
        unreachable();
    }
}

// Deserializes a value, assuming it's serialized for a given version.  (This doesn't
// deserialize any version numbers.)
template <class T>
archive_result_t deserialize_for_version(cluster_version_t version,
                                         read_stream_t *s,
                                         T *thing) {
    switch (version) {
    case cluster_version_t::v1_13:
        return deserialize<cluster_version_t::v1_13>(s, thing);
    case cluster_version_t::v1_13_2_is_latest:
        return deserialize<cluster_version_t::v1_13_2_is_latest>(s, thing);
    default:
        unreachable();
    }
}


// Some serialized_size needs to be visible, apparently, so that
// serialized_size_for_version will actually parse.
template <cluster_version_t W>
size_t serialized_size(const archive_internal::bogus_made_up_type_t &);

template <class T>
size_t serialized_size_for_version(cluster_version_t version,
                                   const T &thing) {
    switch (version) {
    case cluster_version_t::v1_13:
        return serialized_size<cluster_version_t::v1_13>(thing);
    case cluster_version_t::v1_13_2_is_latest:
        return serialized_size<cluster_version_t::v1_13_2_is_latest>(thing);
    default:
        unreachable();
    }
}

// We want to express explicitly whether a given serialization function
// is used for cluster messages or disk serialization in case the latest cluster
// and latest disk versions diverge.
//
// If you see either the INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK
// of INSTANTIATE_SERIALIZE_FOR_DISK macro used somewhere, you know that if you
// change the serialization format of that type that will break the disk format,
// and you should consider writing a deserialize function for the older versions.

#define INSTANTIATE_SERIALIZE_FOR_DISK(typ)                  \
    template void serialize<cluster_version_t::LATEST_DISK>( \
            write_message_t *, const typ &)

#define INSTANTIATE_SERIALIZE_FOR_CLUSTER(typ)           \
    template void serialize<cluster_version_t::CLUSTER>( \
            write_message_t *, const typ &)

#define INSTANTIATE_SERIALIZE_SELF_FOR_DISK(typ)                      \
    template void typ::rdb_serialize<cluster_version_t::LATEST_DISK>( \
            write_message_t *) const

#define INSTANTIATE_SERIALIZE_SELF_FOR_CLUSTER(typ)               \
    template void typ::rdb_serialize<cluster_version_t::CLUSTER>( \
            write_message_t *) const

#ifdef CLUSTER_AND_DISK_VERSIONS_ARE_SAME
#define INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK(typ)  \
    template void serialize<cluster_version_t::CLUSTER>( \
            write_message_t *, const typ &)
#define INSTANTIATE_SERIALIZE_SELF_FOR_CLUSTER_AND_DISK(typ)          \
    template void typ::rdb_serialize<cluster_version_t::CLUSTER>(     \
            write_message_t *) const
#else
#define INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK(typ)      \
    template void serialize<cluster_version_t::CLUSTER>(     \
            write_message_t *, const typ &);                 \
    template void serialize<cluster_version_t::LATEST_DISK>( \
            write_message_t *, const typ &)
#define INSTANTIATE_SERIALIZE_SELF_FOR_CLUSTER_AND_DISK(typ)          \
    template void typ::rdb_serialize<cluster_version_t::CLUSTER>(     \
            write_message_t *) const;                                 \
    template void typ::rdb_serialize<cluster_version_t::LATEST_DISK>( \
            write_message_t *) const
#endif

#define INSTANTIATE_DESERIALIZE_SINCE_v1_13(typ)                                 \
    template archive_result_t deserialize<cluster_version_t::v1_13>(             \
            read_stream_t *, typ *);                                             \
    template archive_result_t deserialize<cluster_version_t::v1_13_2_is_latest>( \
            read_stream_t *, typ *)

#define INSTANTIATE_DESERIALIZE_SELF_SINCE_v1_13(typ)                                     \
    template archive_result_t typ::rdb_deserialize<cluster_version_t::v1_13>(             \
            read_stream_t *s);                                                            \
    template archive_result_t typ::rdb_deserialize<cluster_version_t::v1_13_2_is_latest>( \
            read_stream_t *s)

#define INSTANTIATE_SERIALIZED_SIZE_SINCE_v1_13(typ)                                  \
    template size_t serialized_size<cluster_version_t::v1_13>(const typ &)            \
    template size_t serialized_size<cluster_version_t::v1_13_2_is_latest>(const typ &)

#define INSTANTIATE_SINCE_v1_13(typ)                     \
    INSTANTIATE_SERIALIZE_FOR_CLUSTER_AND_DISK(typ);     \
    INSTANTIATE_DESERIALIZE_SINCE_v1_13(typ)

#define INSTANTIATE_SELF_SINCE_v1_13(typ)                 \
    INSTANTIATE_SERIALIZE_SELF_FOR_CLUSTER_AND_DISK(typ); \
    INSTANTIATE_DESERIALIZE_SELF_SINCE_v1_13(typ)


#endif  // CONTAINERS_ARCHIVE_VERSIONED_HPP_
