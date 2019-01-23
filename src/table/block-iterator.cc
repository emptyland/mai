#include "table/block-iterator.h"
#include "core/key-boundle.h"
#include "core/internal-key-comparator.h"
#include "base/slice.h"
#include "mai/env.h"
#include "glog/logging.h"

namespace mai {

namespace table {
    
using ::mai::base::Slice;
    
BlockIterator::BlockIterator(const core::InternalKeyComparator *ikcmp,
                             const void *block,
                             uint64_t block_size)
    : ikcmp_(DCHECK_NOTNULL(ikcmp))
    , data_base_(static_cast<const char *>(block))
    , data_end_(data_base_ + block_size) {

    n_restarts_ = Slice::SetFixed32(std::string_view(data_end_ - 4, 4));
    DCHECK_GT(n_restarts_, 0);
    
    auto idx = data_end_ - 4 - n_restarts_ * 4;
    restarts_ = reinterpret_cast<const uint32_t *>(idx);

    data_end_ = idx;
}

/*virtual*/ BlockIterator::~BlockIterator() {}

/*virtual*/ bool BlockIterator::Valid() const {
    return error_.ok() &&
    (curr_restart_ >= 0 && curr_restart_ < n_restarts_) &&
    (curr_local_ >= 0 && curr_local_ < local_.size());
}

/*virtual*/ void BlockIterator::SeekToFirst() {
    PrepareRead(0);
    curr_local_   = 0;
    curr_restart_ = 0;
}

/*virtual*/ void BlockIterator::SeekToLast() {
    PrepareRead(n_restarts_ - 1);
    curr_local_   = static_cast<int64_t>(local_.size()) - 1;
    curr_restart_ = static_cast<int64_t>(n_restarts_) - 1;
}

/*virtual*/ void BlockIterator::Seek(std::string_view target) {
    /*
    bool found = false;
    int32_t i;
    std::tuple<std::string, std::string> kv;
    for (i = static_cast<int32_t>(n_restarts_) - 1; i >= 0; i--) {
        Read("", data_base_ + restarts_[i], &kv);
        if (ikcmp_->Compare(target, std::get<0>(kv)) >= 0) {
            found = true;
            break;
        }
    }
    if (!found) {
        i = 0;
    }

    for (; i < static_cast<int32_t>(n_restarts_); i++) {
        PrepareRead(i);
        for (auto j = 0; j < local_.size(); j++) {
            if (ikcmp_->Compare(target, std::get<0>(local_[j])) <= 0) {
                curr_local_   = j;
                curr_restart_ = i;
                return;
            }
        }
    }
    error_ = MAI_NOT_FOUND("Seek()");
    */
    
    std::tuple<std::string, std::string> kv;
    int rv = 0;
    int64_t count = n_restarts_, first = 0;
    while (count > 0) {
        auto it = first;
        auto step = count / 2;
        it += step;
        
        Read("", data_base_ + restarts_[it], &kv);
        rv = ikcmp_->Compare(target, std::get<0>(kv));
        if (!(rv < 0)) {
            first = ++it;
            count -= step + 1;
        } else {
            count = step;
        }
    }
    if (first != 0) {
        first--;
    } if (first >= n_restarts_) {
        first = 0;
    }

    kv = std::make_tuple(target, "");
    for (int64_t i = first; i < n_restarts_; ++i) {
        if (i > first + 1) {
            break;
        }
        PrepareRead(i);
        
        auto iter = std::lower_bound(local_.begin(), local_.end(), kv,
        [this](const auto &lhs, const auto &rhs) {
            return ikcmp_->Compare(std::get<0>(lhs), std::get<0>(rhs)) < 0;
        });
        if (iter != local_.end()) {
            
            curr_local_   = std::distance(local_.begin(), iter);
            curr_restart_ = first;
            return;
        }
    }
    error_ = MAI_NOT_FOUND("Seek()");
}

/*virtual*/ void BlockIterator::Next() {
    if (curr_local_ >= local_.size() - 1) {
        if (curr_restart_ < n_restarts_ - 1) {
            PrepareRead(++curr_restart_);
        } else {
            ++curr_restart_;
        }
        curr_local_ = 0;
        return;
    }
    
    curr_local_++;
}

/*virtual*/ void BlockIterator::Prev() {
    if (curr_local_ == 0) {
        if (curr_restart_ > 0) {
            PrepareRead(--curr_restart_);
        } else {
            --curr_restart_;
        }
        curr_local_ = static_cast<int64_t>(local_.size()) - 1;
        return;
    }
    
    --curr_local_;
}

/*virtual*/ std::string_view BlockIterator::key() const {
    return std::get<0>(local_[curr_local_]);
}
/*virtual*/ std::string_view BlockIterator::value() const {
    return std::get<1>(local_[curr_local_]);
}

/*virtual*/ Error BlockIterator::error() const { return error_; }
    
const char *BlockIterator::PrepareRead(uint64_t i) {
    DCHECK_LT(i, n_restarts_);
    
    const char *p   = data_base_ + restarts_[i];
    const char *end = (i == n_restarts_ - 1) ? data_end_ : data_base_ + restarts_[i + 1];
    
    std::tuple<std::string, std::string> kv;
    std::string last_key;
    local_.clear();
    while (p < end) {
        p = Read(last_key, p, &kv);
        if (error_.fail()) {
            return nullptr;
        }
        last_key = std::get<0>(kv);
        local_.push_back(kv);
    }
    return p;
}

const char *BlockIterator::Read(std::string_view prev_key, const char *start,
                                std::tuple<std::string, std::string> *kv) {
    base::BufferReader reader(std::string_view(start, data_end_ - start));
    uint64_t shared_len = reader.ReadVarint64();
    uint64_t private_len = reader.ReadVarint64();

    std::string key(prev_key.substr(0, shared_len));
    
    // Key: private part
    std::string_view result = reader.ReadString(private_len);
    
    key.append(result);
    
//    core::Tag tag = core::KeyBoundle::ExtractTag(key);
//    if (tag.flag() == core::Tag::kFlagValue) {
//        // Value
//        result = reader.ReadString();
//    } else {
//        result = "";
//    }
    result = reader.ReadString();
    
    *kv = std::make_tuple(key, result);
    return start + reader.position();
}

} // namespace table

} // namespace mai
