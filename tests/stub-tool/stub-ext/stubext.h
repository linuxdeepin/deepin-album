// SPDX-FileCopyrightText: 2020 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef STUBEXT_H
#define STUBEXT_H

//需修改Stub的私用成员函数和成员变量为保护类型
#include <stub-tool/cpp-stub/stub.h>
#include "stub-shadow.h"

#define VADDR(CLASS_NAME,MEMBER_NAME) (typename stub_ext::VFLocator<decltype(&CLASS_NAME::MEMBER_NAME)>::Func)(&CLASS_NAME::MEMBER_NAME)

namespace stub_ext {

class StubExt : public Stub
{
public:
    StubExt() : Stub() {}

    template<typename T, class Lamda>
    void set_lamda(T addr, Lamda lamda)
    {
        Wrapper *wrapper = nullptr;
        auto addr_stub = depictShadow(&wrapper,addr,lamda);
        set(addr, addr_stub);
        char *fn = addrof(addr);
        auto iter = m_result.find(fn);
        if (iter != m_result.end()) {
            m_wrappers.insert(std::make_pair(fn,wrapper));
        } else {
            freeWrapper(wrapper);
        }
    }

    template<typename T>
    void reset(T addr)
    {
        Stub::reset(addr);
        char *fn = addrof(addr);
        auto iter = m_wrappers.find(fn);
        if (iter != m_wrappers.end()) {
            freeWrapper(iter->second);
            m_wrappers.erase(iter);
        }
    }

    ~StubExt()
    {
        for (auto iter = m_wrappers.begin(); iter != m_wrappers.end(); ++iter) {
            freeWrapper(iter->second);
        }
    }
protected:
    std::map<char*, Wrapper*> m_wrappers;
};

}

#endif // STUBEXT_H
