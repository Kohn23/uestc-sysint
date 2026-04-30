/*
 * 功能描述:
 *   验证 O_APPEND 标志下 lseek 对文件偏移量的影响，
 *   明确读、写操作发生的位置。
*/

#include <stdio.h>      /* 提供标准输入输出函数  */
#include <fcntl.h>      /* 提供文件控制选项，如 O_RDWR, O_APPEND   */
#include <unistd.h>     /* 提供 POSIX 系统调用，如 write, lseek, read */

int main() {
    /*
     * 打开文件 "test"：
     * O_RDWR   - 以读写模式打开
     * O_APPEND - 每次写操作前自动将文件偏移量移到末尾
     * O_TRUNC  - 若文件存在，将其长度截断为 0
     * 0644     - 赋予文件拥有者读写权限，组及其他用户只读权限
     */
    int fd = open("test", O_RDWR | O_APPEND | O_TRUNC, 0644);
    if (fd == -1) {
        /* 若打开失败，直接返回 1 表示异常退出 */
        return 1;
    }

    /*
     * 第一次写入：写入 "First word" 共 10 个字节（不含空字符）。
     * 此时文件大小 10，偏移量位于 10 处。
     */
    write(fd, "First word", 10);

    /*
     * 使用 lseek 将文件偏移量移动到文件开头 (0)。
     * 返回值是新的偏移量，这里忽略返回值。
     * 注意：O_APPEND 不影响 lseek 本身，偏移量确实被修改为 0。
     */
    lseek(fd, 0, SEEK_SET);

    /* 准备缓冲区，初始化为全零，确保可读性 */
    char buf[100] = {'\0'};

    /*
     * 第一次读取：从当前偏移量 (0) 开始读取最多 100 字节。
     * 因为文件前 10 字节就是刚刚写入的内容，所以将读出 "First word"。
     */
    ssize_t n = read(fd, buf, 100);
    /* 打印读到的字节数和内容，%s 遇到空字符会停止，但此处无空字符，故完整输出 */
    printf("read returns %ld bytes at: ^%s \n", n, buf);

    /*
     * 第二次写入：尝试写入 "Second word" 共 12 个字节。
     * 尽管 lseek 已将偏移量设为 0，但 O_APPEND 会强制将偏移量移至文件末尾再进行写入。
     * 因此数据被追加到 "First word" 之后，文件总长度变为 10 + 12 = 22。
     */
    write(fd, "Second word", 12);

    /*
     * 第二次读取：当前偏移量处于 22（即文件末尾），读取会立即返回 EOF。
     * 返回值应为 0。
     */
    n = read(fd, buf, 100);
    printf("read returns %ld\n", n);

    /* 再次用 lseek 将偏移量重置到文件开头，准备读取全部内容 */
    lseek(fd, 0, SEEK_SET);

    /*
     * 第三次读取：从 0 开始读取全部 22 字节数据。
     * 缓冲区中前 10 字节为 "First word"，紧跟 12 字节的 "Second word\0"。
     * printf 遇到 \0 停止，因此只显示 "First wordSecond word"，但 n 的值为 22。
     */
    n = read(fd, buf, 100);
    printf("read returns %ld bytes at: ^%s \n", n, buf);

    /* 关闭文件描述符，释放资源 */
    close(fd);

    /* 返回 0 表示程序正常结束 */
    return 0;
}
