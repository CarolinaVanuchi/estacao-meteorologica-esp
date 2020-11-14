#ifndef _EMBBEDED_FILES_
#define _EMBBEDED_FILES_

extern const unsigned char cacert_pem_start[]               asm("_binary_ca_cert_pem_start");
extern const unsigned char cacert_pem_end[]                 asm("_binary_ca_cert_pem_end");

extern const unsigned char prvtkey_pem_start[]              asm("_binary_ca_key_pem_start");
extern const unsigned char prvtkey_pem_end[]                asm("_binary_ca_key_pem_end");

extern const unsigned char index_html_start[]               asm("_binary_index_html_start");
extern const unsigned char index_html_end[]                 asm("_binary_index_html_end");

extern const unsigned char configuration_html_start[]       asm("_binary_configuration_html_start");
extern const unsigned char configuration_html_end[]         asm("_binary_configuration_html_end");

extern const unsigned char index_js_start[]                 asm("_binary_index_js_start");
extern const unsigned char index_js_end[]                   asm("_binary_index_js_end");

extern const unsigned char configuration_js_start[]         asm("_binary_configuration_js_start");
extern const unsigned char configuration_js_end[]           asm("_binary_configuration_js_end");

extern const unsigned char style_css_start[]                asm("_binary_style_css_start");
extern const unsigned char style_css_end[]                  asm("_binary_style_css_end");

#endif