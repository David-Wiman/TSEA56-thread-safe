#include "catch.hpp"
#include "threaded_queue.h"
#include "replacing_buffer.h"

#include <string>
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;
TEST_CASE("Threaded queue") {
    SECTION("int") {
        ThreadedQueue<int> int_queue{};
        CHECK( int_queue.empty() );

        int_queue.enqueue(1);
        int_queue.enqueue(2);
        int_queue.enqueue(3);
        CHECK( int_queue.size() == 3 );
        CHECK( int_queue.dequeue() == 1 );
    }
    SECTION("string") {
        ThreadedQueue<string> str_queue{};
        CHECK( str_queue.empty() );

        str_queue.enqueue("Hello");
        str_queue.enqueue("There!");
        CHECK( str_queue.size() == 2 );
        CHECK( str_queue.dequeue() == "Hello" );
    }
    SECTION("Max size") {
        ThreadedQueue<int> int_queue{1};
        CHECK( int_queue.full() == false );
        int_queue.enqueue(1);
        CHECK( int_queue.full() == true );

        ThreadedQueue<int> no_limit_queue{};
        CHECK( no_limit_queue.full() == false);
        no_limit_queue.enqueue(1);
        CHECK( no_limit_queue.full() == false);
    }
    SECTION("Basic threading") {
        ThreadedQueue<int> int_queue{};

        thread th{[&](ThreadedQueue<int> *int_queue){
            this_thread::sleep_for(chrono::milliseconds(1));
            int_queue->enqueue(1);
        }, &int_queue};

        CHECK( int_queue.size() == 0 );
        CHECK( int_queue.dequeue() == 1 );  // blocks til there is a value
        CHECK( int_queue.size() == 0 );
        th.join();
    }
    SECTION("Max size") {
        ThreadedQueue<int> int_queue{1};

        thread th0{[&](ThreadedQueue<int> *int_queue){
            this_thread::sleep_for(chrono::milliseconds(1));
            int_queue->enqueue(1);
        }, &int_queue};

        thread th1{[&](ThreadedQueue<int> *int_queue){
            this_thread::sleep_for(chrono::milliseconds(2));
            // This will block because the queue is full
            int_queue->enqueue(2);
        }, &int_queue};

        CHECK( int_queue.full() == false );
        CHECK( int_queue.size() == 0 );
        this_thread::sleep_for(chrono::milliseconds(3));
        CHECK( int_queue.full() == true );
        CHECK( int_queue.size() == 1 );

        CHECK( int_queue.dequeue() == 1 );
        this_thread::sleep_for(chrono::milliseconds(1));
        CHECK( int_queue.full() == true );
        CHECK( int_queue.dequeue() == 2 );

        th0.join();
        th1.join();
    }
    SECTION("Waiting") {
        ThreadedQueue<int> int_queue{1};

        thread th0{[&](ThreadedQueue<int> *int_queue){
            int_queue->enqueue(1);
        }, &int_queue};

        thread th1{[&](ThreadedQueue<int> *int_queue){
            this_thread::sleep_for(chrono::milliseconds(2));
            int_queue->dequeue();
        }, &int_queue};

        this_thread::sleep_for(chrono::milliseconds(1));
        CHECK( int_queue.full() == true );
        this_thread::sleep_for(chrono::milliseconds(2));
        CHECK( int_queue.full() == false );

        thread th2{[&](ThreadedQueue<int> *int_queue){
            int_queue->enqueue(1);
        }, &int_queue};

        thread th3{[&](ThreadedQueue<int> *int_queue){
            this_thread::sleep_for(chrono::milliseconds(2));
            int_queue->dequeue();
        }, &int_queue};

        this_thread::sleep_for(chrono::milliseconds(1));
        int_queue.wait_if_full();
        CHECK( int_queue.full() == false );

        th0.join();
        th1.join();
        th2.join();
        th3.join();
    }
}

TEST_CASE("ReplacingBuffer") {
    SECTION("Basics") {
        ReplacingBuffer<int> buffer{};
        int one{1};
        int two{2};
        int three{3};
        CHECK(buffer.full() == false);
        CHECK(buffer.empty() == true);
        buffer.store(one);
        CHECK(buffer.full() == true);
        CHECK(buffer.empty() == false);
        CHECK(buffer.extract() == 1);
        CHECK(buffer.empty() == true);
        buffer.store(two);
        buffer.store(three);
        CHECK(buffer.extract() == 3);
    }
    SECTION("Larger data types and scopes") {
        ReplacingBuffer<string> buffer{};
        {
            string str1{"FIRST"};
            string str2{"SECOND"};
            buffer.store(str1);
            buffer.store(str2);
        }
        {
            CHECK(buffer.extract() == "SECOND");
        }
    }
}
