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
    //Ёто не даст программе скомпилироватьс€ при попытке использовать в качестве типа ключа что-либо, кроме целых чисел
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    //ѕредоставл€ет ссылку на значение словар€ и обеспечивать синхронизацию доступа к нему
    //ћое примечание: это то же, что и шаблон Synchronized (см. в предыдущих примерах)
    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            : guard(bucket.mutex)
            , ref_to_value(bucket.map[key]) {
        }
    };

    // онструктор класса ConcurrentMap<Key, Value> принимает количество подсловарей, на которые надо разбить всЄ пространство ключей
    explicit ConcurrentMap(size_t bucket_count)
        : buckets_(bucket_count) {
    }

    //operator[] должен вести себ€ так же, как аналогичный оператор у map: если ключ key есть в словаре, должен возвращатьс€ объект класса Access,
    //  содержащий ссылку на соответствующее ему значение.
    // ≈сли key в словаре нет, в него надо добавить пару(key, Value()) и вернуть объект класса Access, содержащий ссылку на только что добавленное значение.
    Access operator[](const Key& key) {
        auto& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return { key, bucket };
    }

    //ћетод BuildOrdinaryMap должен сливать вместе части словар€ и возвращать весь словарь целиком.
    // ѕри этом он должен быть потокобезопасным, то есть корректно работать, когда другие потоки выполн€ют операции с ConcurrentMap.
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