#include "MobileDevice.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

static char *the_service;

static void cb(am_device_notification_callback_info *info, void *foo) {
    struct am_device *dev;
    fprintf(stderr, "... %x\n", info->msg);
    if(info->msg == ADNCI_MSG_CONNECTED) {
        dev = info->dev;

        AMDeviceConnect(dev);
        assert(AMDeviceIsPaired(dev));
        assert(!AMDeviceValidatePairing(dev));
        assert(!AMDeviceStartSession(dev));
        
        service_conn_t socket = 0;
        assert(!AMDeviceStartService(dev, CFStringCreateWithCStringNoCopy(NULL, the_service, kCFStringEncodingUTF8, kCFAllocatorNull), &socket, NULL));

        assert(!fcntl(socket, F_SETFL, O_NONBLOCK));
        assert(!fcntl(0, F_SETFL, O_NONBLOCK));

        while(1) {
            char buf[65536];
            fd_set fdset;
            FD_ZERO(&fdset);
            FD_SET(0, &fdset);
            FD_SET(socket, &fdset);
            if(select(socket + 1, &fdset, NULL, NULL, NULL) < 1) break;
            int fd = FD_ISSET(0, &fdset) ? 0 : socket;
            ssize_t bytes = read(fd, buf, sizeof(buf));
            if(bytes <= 0) break;
            if(fd == 0) {
                assert(write(socket, buf, (size_t) bytes) == (size_t) bytes);
            } else {
                // XXX why is this necessary?
                assert(1 == fwrite(buf, (size_t) bytes, 1, stdout));
            }
        }

        exit(0);
    }
}

int main(int argc, char **argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: interact servicename\n");
        return 1;
    }

    the_service = argv[1];

    AMDAddLogFileDescriptor(2);
    am_device_notification *notif;
    assert(!AMDeviceNotificationSubscribe(cb, 0, 0, NULL, &notif));
    CFRunLoopRun();
    return 0;
}
