int str_spilt(char *pData)
{
    //char pData[1024] = ";CMD:wifi;S:幻想動力1234;P:hxdl@1234;;";
    char command[32] = {0};
    char ssid[64] = {0};
    char password[64] = {0};

    printf("QR: [%s]: %ld bytes\n", pData, strlen(pData));

    char delim = pData[0];
    printf("delim[%c]\n", delim);

    char* token = strtok(pData, &delim);
    while (token != NULL) {
        printf("%s\n", token);
        if (0 == strncmp("CMD:wifi", token, 4)) {
            strcpy(ssid, &token[4]);
        } else if (0 == strncmp("S:", token, 2)) {
            strcpy(ssid, &token[2]);
        } else if (0 == strncmp("P:", token, 2)) {
            strcpy(password, &token[2]);
        }
        token = strtok(NULL, &delim);
    }
    printf("QR: CMD:[%s] SSID:[%s] PW:[%s]\n", command, ssid, password);
}