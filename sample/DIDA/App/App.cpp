#include <stdio.h>
#include <string.h>
#include <assert.h>

# include <unistd.h>
# include <pwd.h>
#include <iostream>
#include <fstream>
#include <getopt.h>
#include <regex>
# define MAX_PATH FILENAME_MAX

#include "sgx_urts.h"
#include "sgx_uae_service.h"
#include "App.h"
#include "Enclave_u.h"
#include "dsp.h"

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

typedef struct _sgx_errlist_t {
  sgx_status_t err;
  const char *msg;
  const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Error code returned by sgx_create_enclave */
static sgx_errlist_t sgx_errlist[] = {
    {
        SGX_ERROR_UNEXPECTED,
        "Unexpected error occurred.",
        NULL
    },
    {
        SGX_ERROR_INVALID_PARAMETER,
        "Invalid parameter.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_MEMORY,
        "Out of memory.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_LOST,
        "Power transition occurred.",
        "Please refer to the sample \"PowerTransition\" for details."
    },
    {
        SGX_ERROR_INVALID_ENCLAVE,
        "Invalid enclave image.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ENCLAVE_ID,
        "Invalid enclave identification.",
        NULL
    },
    {
        SGX_ERROR_INVALID_SIGNATURE,
        "Invalid enclave signature.",
        NULL
    },
    {
        SGX_ERROR_OUT_OF_EPC,
        "Out of EPC memory.",
        NULL
    },
    {
        SGX_ERROR_NO_DEVICE,
        "Invalid SGX device.",
        "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards."
    },
    {
        SGX_ERROR_MEMORY_MAP_CONFLICT,
        "Memory map conflicted.",
        NULL
    },
    {
        SGX_ERROR_INVALID_METADATA,
        "Invalid enclave metadata.",
        NULL
    },
    {
        SGX_ERROR_DEVICE_BUSY,
        "SGX device was busy.",
        NULL
    },
    {
        SGX_ERROR_INVALID_VERSION,
        "Enclave version was invalid.",
        NULL
    },
    {
        SGX_ERROR_INVALID_ATTRIBUTE,
        "Enclave was not authorized.",
        NULL
    },
    {
        SGX_ERROR_ENCLAVE_FILE_ACCESS,
        "Can't open enclave file.",
        NULL
    },
    {
        SGX_ERROR_NDEBUG_ENCLAVE,
        "The enclave is signed as product enclave, and can not be created as debuggable enclave.",
        NULL
    },
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

/* Application entry */
int SGX_CDECL main(int argc, char *argv[]) {
  (void) (argc);
  (void) (argv);


  /* Initialize the enclave */
  if (initialize_enclave() < 0) {
    printf("Enter a character before exit ...\n");
    getchar();
    return -1;
  }


  /* Utilize trusted libraries */
  //ecall_libcxx_functions();

  std::cout << "Loading bloom filters" << std::endl;
  std::vector<std::vector<bool> *> bfs = dida_build_bf(argc, argv);
  std::cout << "Done loading bloom filters of length : " << bfs.size() << std::endl;

  for (int x = 0; x < bfs.size(); x++) {
    std::cout << "Encoding " << x << "th bfx" << std::endl;
    std::vector<bool> *bf = bfs[x];
    long bf_size = bf->size();

    std::cout << "BF size :  " << bf_size << std::endl;
    long char_arr_size = (bf_size / 8) + 1;
    unsigned char *bf_data = new unsigned char[char_arr_size];
    std::cout << "array size :  " << bf_size << std::endl;
    std::cout << "Encoding a bf of size " << bf->size() << std::endl;
    for (long i = 0; i < bf_size;) {
      unsigned char val = 0b00000000;
      for (int b = 7; b > -1 && i < bf_size; b--, i++) {
        val |= (bf->at(i) << b);
      }
      bf_data[(i / 8) - 1] = val;
    }
    printf("\n\n Sent\n\n");
    for (int i = 0; i < 40; i++) {
      printf("%s", bf->at(i) ? "1" : "0");
    }
    printf("\n");

    printf("\n\n Sent Chars\n\n");
    for (int i = 0; i < 5; i++) {
      printf("%d,", bf_data[i]);
    }
    printf("\n");

    delete bf;
    std::cout << "Sending a bf of size " << char_arr_size << " to the encalve..." << std::endl;
    sgx_status_t ret = ecall_load_bf(global_eid, bf_data, char_arr_size, bf_size);
    delete[] bf_data;
    if (ret != SGX_SUCCESS) {
      std::cerr << "Failed to add bloom filter to enclave" << std::endl;
    } else {
      printf("Back from enclave\n");
    }

    printf("\n");
  }

  ecall_print_bf_summary(global_eid);

  // PARSE ARGS
  const char shortopts[] = "s:l:b:p:j:d:h:i:r";

  enum { OPT_HELP = 1, OPT_VERSION };

  int bmer = 16;
  int bmer_step = -1;
  int nhash = 5;
  int se = 0;
  int fq = 0;
  int pnum = 12;
  unsigned threads = 0;
  int alen = 20;
  unsigned ibits = 8;

  const struct option longopts[] = {
      {"threads", required_argument, NULL, 'j'},
      {"partition", required_argument, NULL, 'p'},
      {"bmer", required_argument, NULL, 'b'},
      {"alen", required_argument, NULL, 'l'},
      {"step", required_argument, NULL, 's'},
      {"hash", required_argument, NULL, 'h'},
      {"bit", required_argument, NULL, 'i'},
      {"rebf", no_argument, NULL, 0},
      {"se", no_argument, &se, 1},
      {"fq", no_argument, &fq, 1},
      {"help", no_argument, NULL, OPT_HELP},
      {"version", no_argument, NULL, OPT_VERSION},
      {NULL, 0, NULL, 0}
  };

  bool die = false;
  std::string blPath;

  for (int c; (c = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1;) {
    std::istringstream arg(optarg != NULL ? optarg : "");
    std::cout << "PARAM " << c << std::endl;
    switch (c) {
      case '?':die = true;
        break;
      case 'j':arg >> threads;
        break;
      case 'b':arg >> bmer;
        break;
      case 'p':arg >> pnum;
        break;
      case 'l':arg >> alen;
        break;
      case 's':arg >> bmer_step;
        break;
      case 'h':arg >> nhash;
        break;
      case 'i':arg >> ibits;
        break;
    }
  }

  if (bmer <= 0)
    bmer = 3 * alen / 4;

  if (bmer_step <= 0)
    bmer_step = alen - bmer + 1;

  std::cerr << "num-hash=" << nhash << "\n";
  std::cerr << "bit-item=" << ibits << "\n";
  std::cerr << "bmer-step=" << bmer_step << "\n";
  std::cerr << "bmer=" << bmer << "\n";
  std::cerr << "alen=" << alen << "\n";
  std::cerr << "pnum=" << pnum << "\n";
  // PARSE ARGS

  // now dispatch the reads
  std::cerr << "Dispatching file : " << argv[argc - 1] << std::endl;

  ecall_start_dispatch(global_eid, bmer,
                       bmer_step,
                       nhash,
                       se,
                       fq,
                       pnum);

  std::ifstream t(argv[argc - 1]);
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());

  std::cerr << "Dispatch file length: " << str.size() << std::endl;
  sgx_status_t ret = ecall_load_data(global_eid, const_cast<char *>(str.c_str()), str.length());
  if (ret != SGX_SUCCESS) {
    std::cerr << "Failed to dispatch file : " << ret << std::endl;
  }
  printf("Info: DIDASGX successfully returned.\n");

  /* Destroy the enclave */
  sgx_destroy_enclave(global_eid);
  return 0;
}

