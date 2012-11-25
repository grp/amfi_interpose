
#include <CoreFoundation/CoreFoundation.h>
#include <launch.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <glob.h>
#include <vproc.h>

#include <sys/sysctl.h>
#include <syslog.h>


#define DYLD_INTERPOSE(_replacement,_replacee) \
        __attribute__((used)) static struct{ const void* replacement; const void* replacee; } _interpose_##_replacee \
        __attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacement, (const void*)(unsigned long)&_replacee };


extern int MISValidateSignature(CFStringRef path, CFDictionaryRef options);

int replaced_MISValidateSignature(CFStringRef string, CFDictionaryRef dictionary) {
    syslog(LOG_EMERG, "amfi_interpose: validating!");
    CFShow(string);
    CFShow(dictionary);
    syslog(LOG_EMERG, "allowing any signature from interpose!");

    return 0;
}

DYLD_INTERPOSE(replaced_MISValidateSignature, MISValidateSignature);

mach_port_t makeport() {
    kern_return_t ret;

    mach_port_t port;
    if ((ret = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &port)) != KERN_SUCCESS) {
        syslog(LOG_EMERG, "service port: %s\n", mach_error_string(ret));
    }

    if ((ret = mach_port_insert_right(mach_task_self(), port, port, MACH_MSG_TYPE_MAKE_SEND)) != KERN_SUCCESS) {
        syslog(LOG_EMERG, "mach_port_insert_right: %s\n", mach_error_string(ret));
    }

    if ((ret = host_set_special_port(mach_host_self(), 18, port)) != KERN_SUCCESS) {
        syslog(LOG_EMERG, "host_set_special_port: %s\n", mach_error_string(ret));
    }

    syslog(LOG_EMERG, "took over special port 18? the ret was %x", ret);

    return port;
}

launch_data_t replaced_launch_msg(const launch_data_t t) {
    syslog(LOG_EMERG, "replaced_launch_msg");
    syslog(LOG_EMERG, "launch_msg: %s", (char*)t);
    return 0xBEEF;
}

DYLD_INTERPOSE(replaced_launch_msg, launch_msg);

launch_data_type_t replaced_launch_data_get_type(const launch_data_t t) {
    syslog(LOG_EMERG, "replaced_launch_data_get_type");
    if (t == 0xBEEF) return 1;
    if (t == 0xDEAD) return 0xA;

    return 0;
}

DYLD_INTERPOSE(replaced_launch_data_get_type, launch_data_get_type);

launch_data_t replaced_launch_data_dict_lookup(const launch_data_t t, const char *n) {
    syslog(LOG_EMERG, "replaced_launch_data_dict_lookup");
    if (strcmp(n, "MachServices") == 0) return 0xBEEF;
    return 0xDEAD;
}

DYLD_INTERPOSE(replaced_launch_data_dict_lookup, launch_data_dict_lookup);

mach_port_t replaced_launch_data_get_machport(const launch_data_t t) {
    syslog(LOG_EMERG, "replaced_launch_data_get_machport");
    return makeport();
}

DYLD_INTERPOSE(replaced_launch_data_get_machport, launch_data_get_machport);

__attribute__((constructor)) static void amfi_interpose_init() {
    // this makes it tethered so you don't mess your device up
    unlink("/var/run/mobile_image_mounter/developer.dimage");

    syslog(LOG_EMERG, "hello there, amfi_interpose here");

    // let launchctl do its magic
    pid_t pid;
    int status;

    const char* launchctl_unload_args[] = { "launchctl", "unload", "/System/Library/LaunchDaemons/com.apple.MobileFileIntegrity.plist", NULL };
    posix_spawn(&pid, "/bin/launchctl", NULL, NULL, (char* const*) launchctl_unload_args, NULL);
    waitpid(pid, &status, WEXITED);

    syslog(LOG_EMERG, "allowing amfi to actually start");
}





