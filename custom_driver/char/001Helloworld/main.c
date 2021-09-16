#include <linux/module.h>

static int __init hello_entry(void)
{

	pr_info("Hello Linux\n");
	return 0;
}

static void __exit hello_exit(void)
{
        pr_info("Good Bye Crule world\n");
}

module_init(hello_entry);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ABHISHEK CHAUHAN");
MODULE_DESCRIPTION("This is simple hello world module");
MODULE_INFO(board, "Begalbone Black REV C");
