#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "project.h"

#ifdef USE_MESSAGE_OVER_LINUX_MQUEUE
void test_messaging_over_linux_mq(void ** state)
{
    x_queue_t test_queue;
    msg_t msg_send = {0}, msg_received= {0};

    /* Test queue creation */
    assert_int_equal(msg_create_LINUX_mq("/test", 10, &test_queue), 0);

    /* Populate the msg to send and test message sending */
    msg_send.src = 0;
    msg_send.dst = 1;
    msg_send.type = 2;
    strncpy(msg_send.content, "abc", 3);
    assert_int_equal(msg_send_LINUX_mq(&test_queue, &msg_send), 0);

    /* Test message receiving */
    assert_int_equal(msg_receive_LINUX_mq(&test_queue, &msg_received), 0);
    assert_memory_equal(&msg_received, &msg_send, sizeof(msg_t));

    /* Test queue destroy */
    assert_int_equal(msg_destroy_LINUX_mq(&test_queue), 0);
}
#endif

int main(int argc, char ** argv)
{
    const struct CMUnitTest tests[] =
    {
#ifdef USE_MESSAGE_OVER_LINUX_MQUEUE
        cmocka_unit_test(test_messaging_over_linux_mq),
#endif
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
