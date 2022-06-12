//------------------------------------------------------------------------------
// This source file is part of the Dispatch open source project
//
// Copyright (c) 2022 - 2022 Dispatch authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//------------------------------------------------------------------------------

#include "DispatchTests.h"
#include <dispatch/dispatch.h>

DispatchGroup* g;
DispatchQueue* q;

static void test_concat() {
	g->enter();
	q->async(^{
		const char* buffer1 = "This is buffer1 ";
		size_t size1 = 17;
		const char* buffer2 = "This is buffer2 ";
		size_t size2 = 17;
		__block bool buffer2_destroyed = false;

        auto data1 = new DispatchData(buffer1, size1);
        auto data2 = new DispatchData(buffer2, size2, *q, ^{
			buffer2_destroyed = true;
		});

        auto concat = new DispatchData();
        concat->append(*data1);
        concat->append(*data2);

		delete data1;
        delete data2;

        CHECK_MESSAGE(concat->count() == 34, "Data size of concatenated dispatch data");

        concat->withUnsafeBytes(^(const void* contig, size_t contig_size) {
            CHECK_MESSAGE(contig_size == 34, "Contiguous memory size");
        });

		delete concat;

        q->async(^{
            auto destroyed = buffer2_destroyed;
            CHECK_MESSAGE(destroyed==true, "Buffer2 destroyed");
			g->leave();
		});
	});
}

static void test_cleanup() // <rdar://problem/9843440>
{
	static char buffer4[16];
	g->enter();
	q->async(^{
		void *buffer3 = malloc(1024);
        auto data3 = new DispatchData(buffer3, 0, *q, DispatchData::Deallocator::FREE);

		__block bool buffer4_destroyed = false;
        auto data4 = new DispatchData(buffer4, 16, *q, ^{
			buffer4_destroyed = true;
		});

		delete data3;
		delete data4;
        q->async(^{
            auto destroyed = buffer4_destroyed;
            CHECK_MESSAGE(destroyed==true, "buffer4 destroyed");
			g->leave();
		});
	});
}


TEST_CASE("Dispatch Data") {
    g = new DispatchGroup();
    q = new DispatchQueue("tech.shifor.Dispatch.DispatchDataTests");
    __block auto semaphore = DispatchSemaphore(0);

    test_concat();
	test_cleanup();

    g->notify(*q, ^{
        delete q;
        semaphore.signal();
    });

    semaphore.wait();
    delete g;
}