#include "table/block-iterator.h"
#include "core/internal-key-comparator.h"
#include "base/slice.h"
#include "mai/env.h"
#include "glog/logging.h"

namespace mai {

namespace table {
    
#define TRY_RUN0(expr) \
    (expr); \
    if (reader_.error().fail()) { \
        error_ = reader_.error(); \
        return 0; \
    } (void)0
    
BlockIterator::BlockIterator(const core::InternalKeyComparator *ikcmp,
              RandomAccessFile *file,
              uint64_t block_offset, uint64_t block_size)
    : ikcmp_(DCHECK_NOTNULL(ikcmp))
    , reader_(DCHECK_NOTNULL(file))
    , data_base_(block_offset) {

    uint64_t offset = block_offset + block_size - 4;
    n_restarts_ = reader_.ReadFixed32(offset);
    if (reader_.error().fail()) {
        error_ = reader_.error();
        return;
    }
    DCHECK_GT(n_restarts_, 0);
    
    restarts_.reset(new uint32_t[n_restarts_]);
    offset -= n_restarts_ * 4;
    std::string_view result = reader_.Read(offset, n_restarts_ * 4);
    if (reader_.error().fail()) {
        error_ = reader_.error();
        return;
    }
    data_end_ = offset;
    memcpy(restarts_.get(), result.data(), result.size());
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
    bool found = false;
    int32_t i;
    std::tuple<std::string, std::string> kv;
    for (i = static_cast<int32_t>(n_restarts_) - 1; i >= 0; i--) {
        uint64_t offset = data_base_ + restarts_[i];
        
        Read("", offset, &kv);
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
    
uint64_t BlockIterator::PrepareRead(uint64_t i) {
    DCHECK_LT(i, n_restarts_);
    
    uint64_t offset = data_base_ + restarts_[i];
    uint64_t end    = (i == n_restarts_ - 1) ? data_end_ : data_base_ + restarts_[i + 1];
    
    std::tuple<std::string, std::string> kv;
    std::string last_key;
    local_.clear();
    while (offset < end) {
        offset = Read(last_key, offset, &kv);
        if (error_.fail()) {
            return 0;
        }
        last_key = std::get<0>(kv);
        local_.push_back(kv);
    }
    return offset;
}

uint64_t BlockIterator::Read(std::string_view prev_key, uint64_t offset,
                             std::tuple<std::string, std::string> *kv) {

    
    size_t len = 0;
    uint64_t shared_len, private_len;
    TRY_RUN0(shared_len = reader_.ReadVarint64(offset, &len));
    offset += len;
    TRY_RUN0(private_len = reader_.ReadVarint64(offset, &len));
    offset += len;
    
    std::string key(prev_key.substr(0, shared_len));
    
    std::string_view result;
    std::string scratch;
    TRY_RUN0(result = reader_.Read(offset, private_len, &scratch));
    offset += result.size();
    
    key.append(result);
    
    uint64_t value_len;
    TRY_RUN0(value_len = reader_.ReadVarint64(offset, &len));
    offset += len;
    TRY_RUN0(result = reader_.Read(offset, value_len, &scratch));
    offset += result.size();

    *kv = std::make_tuple(key, result);
    return offset;
}

} // namespace table

} // namespace mai
