//
// Created by lidong on 2025/1/27.
//

#ifndef KRKR2_DEFER_H
#define KRKR2_DEFER_H

#include <utility>

#define SCOPE_GUARD_CONCATENATE_IMPL(s1, s2) s1##s2
#define SCOPE_GUARD_CONCATENATE(s1, s2) SCOPE_GUARD_CONCATENATE_IMPL(s1, s2)
// ScopeGuard for C++11
namespace clover {

    template <typename Fun>
    class ScopeGuard {
    public:
        explicit ScopeGuard(Fun &&f) :
            _fun(std::forward<Fun>(f)), _active(true) {}

        void dismiss() { _active = false; }

        ScopeGuard() = delete;

        ScopeGuard(const ScopeGuard &) = delete;

        ScopeGuard &operator=(const ScopeGuard &) = delete;
        ScopeGuard(ScopeGuard &&rhs) noexcept :
            _fun(std::move(rhs._fun)), _active(rhs._active) {
            rhs.dismiss();
        }
        ~ScopeGuard() noexcept {
            if(_active) {
                try {
                    _fun();
                } catch(...) {
                    std::terminate();
                } // 或记录日志
            }
        }

    private:
        Fun _fun;
        bool _active;
    };

    template <typename Fun>
    auto make_scope_guard(Fun &&f) {
        return ScopeGuard<Fun>(std::forward<Fun>(f));
    }

} // namespace clover

// Helper macro
#define DEFER(code)                                                            \
    auto SCOPE_GUARD_CONCATENATE(_sg_, __LINE__) =                             \
        ::clover::make_scope_guard([=]() { code; })
#endif // KRKR2_DEFER_H
