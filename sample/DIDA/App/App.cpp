#include <assert.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <regex>
#define MAX_PATH FILENAME_MAX

#include "App.h"
#include "Enclave_u.h"
#include "TrustedLibrary/LibcxxDsp.h"
#include "TrustedLibrary/LibcxxMrg.h"
#include "sgx_uae_service.h"
#include "sgx_urts.h"

using namespace std::chrono;

/* DIDA modes*/
std::string dida_dsp = "dsp";
std::string dida_mrg = "mrg";

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {SGX_ERROR_UNEXPECTED,
     "Unexpected error occurred.",
     NULL},
    {SGX_ERROR_INVALID_PARAMETER,
     "Invalid parameter.",
     NULL},
    {SGX_ERROR_OUT_OF_MEMORY,
     "Out of memory.",
     NULL},
    {SGX_ERROR_ENCLAVE_LOST,
     "Power transition occurred.",
     "Please refer to the sample \"PowerTransition\" for details."},
    {SGX_ERROR_INVALID_ENCLAVE,
     "Invalid enclave image.",
     NULL},
    {SGX_ERROR_INVALID_ENCLAVE_ID,
     "Invalid enclave identification.",
     NULL},
    {SGX_ERROR_INVALID_SIGNATURE,
     "Invalid enclave signature.",
     NULL},
    {SGX_ERROR_OUT_OF_EPC,
     "Out of EPC memory.",
     NULL},
    {SGX_ERROR_NO_DEVICE,
     "Invalid SGX device.",
     "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."},
    {SGX_ERROR_MEMORY_MAP_CONFLICT,
     "Memory map conflicted.",
     NULL},
    {SGX_ERROR_INVALID_METADATA,
     "Invalid enclave metadata.",
     NULL},
    {SGX_ERROR_DEVICE_BUSY,
     "SGX device was busy.",
     NULL},
    {SGX_ERROR_INVALID_VERSION,
     "Enclave version was invalid.",
     NULL},
    {SGX_ERROR_INVALID_ATTRIBUTE,
     "Enclave was not authorized.",
     NULL},
    {SGX_ERROR_ENCLAVE_FILE_ACCESS,
     "Can't open enclave file.",
     NULL},
    {SGX_ERROR_NDEBUG_ENCLAVE,
     "The enclave is signed as product enclave, and can not be created as debuggable enclave.",
     NULL},
};

/* Check error conditions for loading enclave */
void print_error_message(sgx_status_t ret) {
    size_t idx = 0;
    size_t ttl = sizeof sgx_errlist / sizeof sgx_errlist[0];

    for (idx = 0; idx < ttl; idx++) {
        if (ret == sgx_errlist[idx].err) {
            if (NULL != sgx_errlist[idx].sug)
                printf("Info: %s\n", sgx_errlist[idx].sug);
            printf("Error: %s\n", sgx_errlist[idx].msg);
            break;
        }
    }

    if (idx == ttl)
        printf("Error: Unexpected error occurred.\n");
}

/* Initialize the enclave:
 *   Step 1: try to retrieve the launch token saved by last transaction
 *   Step 2: call sgx_create_enclave to initialize an enclave instance
 *   Step 3: save the launch token if it is updated
 */
int initialize_enclave(void) {
    char token_path[MAX_PATH] = {'\0'};
    sgx_launch_token_t token = {0};
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    int updated = 0;

    /* Step 1: try to retrieve the launch token saved by last transaction
   *         if there is no token, then create a new one.
   */
    /* try to get the token saved in $HOME */
    const char *home_dir = getpwuid(getuid())->pw_dir;

    if (home_dir != NULL &&
        (strlen(home_dir) + strlen("/") + sizeof(TOKEN_FILENAME) + 1) <= MAX_PATH) {
        /* compose the token path */
        strncpy(token_path, home_dir, strlen(home_dir));
        strncat(token_path, "/", strlen("/"));
        strncat(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME) + 1);
    } else {
        /* if token path is too long or $HOME is NULL */
        strncpy(token_path, TOKEN_FILENAME, sizeof(TOKEN_FILENAME));
    }

    FILE *fp = fopen(token_path, "rb");
    if (fp == NULL && (fp = fopen(token_path, "wb")) == NULL) {
        printf("Warning: Failed to create/open the launch token file \"%s\".\n", token_path);
    }

    if (fp != NULL) {
        /* read the token from saved file */
        size_t read_num = fread(token, 1, sizeof(sgx_launch_token_t), fp);
        if (read_num != 0 && read_num != sizeof(sgx_launch_token_t)) {
            /* if token is invalid, clear the buffer */
            memset(&token, 0x0, sizeof(sgx_launch_token_t));
            printf("Warning: Invalid launch token read from \"%s\".\n", token_path);
        }
    }
    /* Step 2: call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, &token, &updated, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        if (fp != NULL) fclose(fp);
        return -1;
    }

    /* Step 3: save the launch token if it is updated */
    if (updated == FALSE || fp == NULL) {
        /* if the token is not updated, or file handler is invalid, do not perform saving */
        if (fp != NULL) fclose(fp);
        return 0;
    }

    /* reopen the file with write capablity */
    fp = freopen(token_path, "wb", fp);
    if (fp == NULL) return 0;
    size_t write_num = fwrite(token, 1, sizeof(sgx_launch_token_t), fp);
    if (write_num != sizeof(sgx_launch_token_t))
        printf("Warning: Failed to save launch token to \"%s\".\n", token_path);
    fclose(fp);
    return 0;
}

/* OCall functions */
void ocall_print_string(const char *str) {
    /* Proxy/Bridge will check the length and null-terminate
   * the input string to prevent buffer overflow.
   */
    printf("%s", str);
}

void ocall_print_file(const char *str, const char *file, int append) {
    std::ofstream stream;

    stream.open(std::string(file), append == 0 ? std::ofstream::out : std::ofstream::app);

    std::cout << "Writing to file " << std::string(file) << std::endl;
    if (!stream)
        std::cout << "Opening file failed" << std::endl;
    // use operator<< for clarity
    stream << std::string(str) << std::endl;
    // test if write was succesful - not *really* necessary
    if (!stream)
        std::cout << "Write failed" << std::endl;

    stream.close();
}

/* Application entry */
int SGX_CDECL main(int argc, char *argv[]) {
    (void)(argc);
    (void)(argv);

    auto start = high_resolution_clock::now();

    /* Initialize the enclave */
    if (initialize_enclave() < 0) {
        printf("Enter a character before exit ...\n");
        getchar();
        return -1;
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(stop - start);

    printf("Enclave initialized in %ld ms\n", duration.count());

    // determining the DIDA command
    std::string dida_command = std::string(argv[1]);

    if (dida_command.compare(dida_dsp) == 0) {
        dsp(argc, argv, global_eid);
    } else if (dida_command.compare(dida_mrg) == 0) {
        mrg(argc, argv, global_eid);
    } else {
        //unknown command
        printf("Unknown command %s", dida_command.c_str());
    }

    /* Utilize trusted libraries */

    stop = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(stop - start);
    printf("Info: DIDASGX successfully returned in %ld ms.\n", duration.count());

    /* Destroy the enclave */
    start = high_resolution_clock::now();
    sgx_destroy_enclave(global_eid);
    stop = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(stop - start);
    printf("Info: Enclave Destroyed in %ld ms.\n", duration.count());

    return 0;
}
