/*
* Copyright (c) 2019-2025 Allwinner Technology Co., Ltd. ALL rights reserved.
*
* Allwinner is a trademark of Allwinner Technology Co.,Ltd., registered in
* the the people's Republic of China and other countries.
* All Allwinner Technology Co.,Ltd. trademarks are used with permission.
*
* DISCLAIMER
* THIRD PARTY LICENCES MAY BE REQUIRED TO IMPLEMENT THE SOLUTION/PRODUCT.
* IF YOU NEED TO INTEGRATE THIRD PARTY’S TECHNOLOGY (SONY, DTS, DOLBY, AVS OR MPEGLA, ETC.)
* IN ALLWINNERS’SDK OR PRODUCTS, YOU SHALL BE SOLELY RESPONSIBLE TO OBTAIN
* ALL APPROPRIATELY REQUIRED THIRD PARTY LICENCES.
* ALLWINNER SHALL HAVE NO WARRANTY, INDEMNITY OR OTHER OBLIGATIONS WITH RESPECT TO MATTERS
* COVERED UNDER ANY REQUIRED THIRD PARTY LICENSE.
* YOU ARE SOLELY RESPONSIBLE FOR YOUR USAGE OF THIRD PARTY’S TECHNOLOGY.
*
*
* THIS SOFTWARE IS PROVIDED BY ALLWINNER"AS IS" AND TO THE MAXIMUM EXTENT
* PERMITTED BY LAW, ALLWINNER EXPRESSLY DISCLAIMS ALL WARRANTIES OF ANY KIND,
* WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING WITHOUT LIMITATION REGARDING
* THE TITLE, NON-INFRINGEMENT, ACCURACY, CONDITION, COMPLETENESS, PERFORMANCE
* OR MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* IN NO EVENT SHALL ALLWINNER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
* NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS, OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
* OF THE POSSIBILITY OF SUCH DAMAGE.
*/
// Copyright 2007-2010 Baptiste Lepilleur and The JsonCpp Authors
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE

#ifndef CPPTL_JSON_ALLOCATOR_H_INCLUDED
#define CPPTL_JSON_ALLOCATOR_H_INCLUDED

#include <cstring>
#include <memory>

#pragma pack(push, 8)

namespace Json {
template<typename T>
class SecureAllocator {
	public:
		// Type definitions
		using value_type      = T;
		using pointer         = T*;
		using const_pointer   = const T*;
		using reference       = T&;
		using const_reference = const T&;
		using size_type       = std::size_t;
		using difference_type = std::ptrdiff_t;

		/**
		 * Allocate memory for N items using the standard allocator.
		 */
		pointer allocate(size_type n) {
			// allocate using "global operator new"
			return static_cast<pointer>(::operator new(n * sizeof(T)));
		}

		/**
		 * Release memory which was allocated for N items at pointer P.
		 *
		 * The memory block is filled with zeroes before being released.
		 * The pointer argument is tagged as "volatile" to prevent the
		 * compiler optimizing out this critical step.
		 */
		void deallocate(volatile pointer p, size_type n) {
			std::memset(p, 0, n * sizeof(T));
			// free using "global operator delete"
			::operator delete(p);
		}

		/**
		 * Construct an item in-place at pointer P.
		 */
		template<typename... Args>
		void construct(pointer p, Args&&... args) {
			// construct using "placement new" and "perfect forwarding"
			::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
		}

		size_type max_size() const {
			return size_t(-1) / sizeof(T);
		}

		pointer address( reference x ) const {
			return std::addressof(x);
		}

		const_pointer address( const_reference x ) const {
			return std::addressof(x);
		}

		/**
		 * Destroy an item in-place at pointer P.
		 */
		void destroy(pointer p) {
			// destroy using "explicit destructor"
			p->~T();
		}

		// Boilerplate
		SecureAllocator() {}
		template<typename U> SecureAllocator(const SecureAllocator<U>&) {}
		template<typename U> struct rebind { using other = SecureAllocator<U>; };
};


template<typename T, typename U>
bool operator==(const SecureAllocator<T>&, const SecureAllocator<U>&) {
	return true;
}

template<typename T, typename U>
bool operator!=(const SecureAllocator<T>&, const SecureAllocator<U>&) {
	return false;
}

} //namespace Json

#pragma pack(pop)

#endif // CPPTL_JSON_ALLOCATOR_H_INCLUDED
