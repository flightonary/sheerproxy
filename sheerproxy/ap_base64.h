#ifndef AP_BASE64_H
#define AP_BASE64_H

int ap_base64decode_len(const char *);
int ap_base64decode(char *, const char *);
int ap_base64encode_len(int);
int ap_base64encode(char *, const char *, int);

#endif    /* !AP_BASE64_H */
