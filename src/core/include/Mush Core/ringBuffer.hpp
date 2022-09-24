#ifndef INPUTRINGBUFFER_HPP
#define INPUTRINGBUFFER_HPP

//#define _BUF_DEBUG

//#include "ConfigStruct.hpp"

#include <memory>
#include <mutex>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <condition_variable>

#include "mushLog.hpp"
#include "mush-core-dll.hpp"
#include "mushBuffer.hpp"

using std::shared_ptr;
using std::make_shared;
using std::mutex;
namespace mush {
    class opencl;
    class doubleBuffer;
}

namespace cl {
	class CommandQueue;
}
/* mush::ringBuffer
 * Shockingly, there's nothing hdr about it at all!
 * It is however, /definitely/ a ring buffer
 */
namespace mush {
	class MUSHEXPORTS_API ringBuffer {
    public:
        ringBuffer() : killswitch{ false }, running{ true }, _unlockCount{ 0 } {

        }

        ~ringBuffer() {
            
        }
        
        /* addItem
         * pushes a pointer and associated metadata into the buffer
         */
        void addItem(mush::buffer);
		void addItem(cl::Image2D *);
		void addItem(cl::Buffer *);
		void addItem(void *);
		void addItem(const void *);

        /* destroy
         * sets the killswitch with an is-run check
         */
        virtual void destroy();
        
        /* inLock
         * gets a locked input pointer
         */
        virtual mush::buffer& inLock();
		virtual mush::buffer& inLock(const uint32_t&);
        
        /* inUnlock
         * releases the last locked pointer
         */
        virtual void inUnlock();
        
        /* outLock
         * gets a locked output pointer; optional nanosecond timeout
         */
        virtual const mush::buffer outLock();
        virtual const mush::buffer outLock(int64_t);
        //virtual void const * outLock(int64_t, uint32_t&);
        virtual const mush::buffer outLock(int64_t, size_t&);
        
        /* outUnlock
         * releases the last locked output pointer
         */

        virtual void outUnlock();
        
        /* good
         * checks all is well; can be looped on
         */
        virtual bool good();
        
        /* release
         * sends the all-clear
         */
        virtual void release();
        
        /* kill
         * sends a more forceful all-clear
         */
        void kill();
        
        /* addRepeat
         * replaces the doubleBuffer
         */
        void addRepeat();
        void removeRepeat();
        
        /* resetRepeat
         * resets the repeat counter
         */
        void resetRepeats();
        
        /* auto-drops the output
         * deprecated
         */
        virtual void null();
        
        /* auto-drops the output
         * deprecated
         */
        virtual void denull();
        
        //std::shared_ptr<ringBuffer> addThreadSafeRepeat();
        
        size_t takeFrameToken();

		void disable();
		void enable();
        
		/*
		 * gets the number of buffers
		 */
		size_t getBufferCount() const {
			return mem.size();
		}

    protected:
        virtual void _addItem(mush::buffer buf) {
            mem.push_back(buf);
        }
        
        virtual mush::buffer& _getMem(uint8_t id) {
            return mem[id];
        }
            
        bool isRunning() {
            return running;
        }
        
        int getBuffers() const {
            return _buffers;
        }
        
        // should be private
        std::vector<bool> empty;
        std::vector<std::shared_ptr<std::condition_variable>> outConds;
        unsigned char next = 0;
        unsigned char now = 0;
        
        std::mutex token_mutex;
        std::condition_variable token_cond;
        std::vector<bool> taken_frame_tokens;
        
        //bool _null = false;

    private:
        mush::buffer end_response = mush::buffer{};
        int _buffers = 0;
        
        int _repeats = 0;
		std::vector<int> _repeats_set;
        std::atomic<int> _unlockCount;
        
        bool ready = false;
        
        std::mutex readyMutex;
        std::condition_variable readyCondition;

        std::vector<mush::buffer> mem;
        
        std::vector<std::shared_ptr<mutex>> locks;
        //std::vector<std::unique_lock<std::mutex>> held_locks;

		std::vector<uint32_t> frame_ids;
        
        std::atomic<bool> running;
        std::atomic<bool> killswitch;

        std::shared_ptr<mush::doubleBuffer> doubleBuff = nullptr;
        bool useDoubleBuff = false;
    #ifdef _BUF_DEBUG
        std::vector<std::string> inL, inU, outL, outU;
    #endif

		bool _disabled_output = false;
		int _disabled_repeats = 0;
		
    };
}

#endif
