#include "parentR2_head.c"
#include "seam_host_jamesjohnson.c"
#include "seam_keys.c"
#include "seam_x509.c"

#define TESTNAME "parentR2"

static void init_local_interface(void)
{
    init_jamesjohnson_interface();
}

static void init_fake_secrets(void)
{
    /* nothing for parentR2 */
}

#include "parentR2_main.c"

 /*
 * Local Variables:
 * c-style: pluto
 * c-basic-offset: 4
 * compile-command: "make check"
 * End:
 */
