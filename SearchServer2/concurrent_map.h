#pragma once

#include <map>
#include <vector>
#include <mutex>

using namespace std::string_literals;
template <typename Key, typename Value>
class ConcurrentMap {
private:
    struct Bucket {
        std::mutex mutex;
        std::map<Key, Value> map;
    };

public:
    //��� �� ���� ��������� ���������������� ��� ������� ������������ � �������� ���� ����� ���-����, ����� ����� �����
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    //������������� ������ �� �������� ������� � ������������ ������������� ������� � ����
    //��� ����������: ��� �� ��, ��� � ������ Synchronized (��. � ���������� ��������)
    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            : guard(bucket.mutex)
            , ref_to_value(bucket.map[key]) {
        }
    };

    //����������� ������ ConcurrentMap<Key, Value> ��������� ���������� �����������, �� ������� ���� ������� �� ������������ ������
    explicit ConcurrentMap(size_t bucket_count)
        : buckets_(bucket_count) {
    }

    //operator[] ������ ����� ���� ��� ��, ��� ����������� �������� � map: ���� ���� key ���� � �������, ������ ������������ ������ ������ Access,
    //  ���������� ������ �� ��������������� ��� ��������.
    // ���� key � ������� ���, � ���� ���� �������� ����(key, Value()) � ������� ������ ������ Access, ���������� ������ �� ������ ��� ����������� ��������.
    Access operator[](const Key& key) {
        auto& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return { key, bucket };
    }

    //����� BuildOrdinaryMap ������ ������� ������ ����� ������� � ���������� ���� ������� �������.
    // ��� ���� �� ������ ���� ����������������, �� ���� ��������� ��������, ����� ������ ������ ��������� �������� � ConcurrentMap.
    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : buckets_) {
            std::lock_guard g(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

    size_t Erase(const Key& key) {
        size_t mapId = static_cast<uint64_t>(key) % buckets_.size();
        auto result = buckets_[mapId].map.erase(key);
        return result;
    }

private:
    std::vector<Bucket> buckets_;
};