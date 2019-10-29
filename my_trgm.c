#ifndef STANDALONE
#ifdef STANDARD
/* STANDARD is defined, don't use any mysql functions */
#include <string.h>
#ifdef __WIN__
typedef unsigned __int64 ulonglong; /* Microsofts 64 bit types */
typedef __int64 longlong;
#else
typedef unsigned long long ulonglong;
typedef long long longlong;
#endif /*__WIN__*/
#else
#include <my_global.h>
#include <my_sys.h>
#if defined(MYSQL_SERVER)
#include <m_string.h>
#else
/* when compiled as standalone */
#include <string.h>
#endif
#endif
#include <ctype.h>
#include <mysql.h>

double calc_similarity(char* str1, char* str2, int len1, int len2);

my_bool trgm_similarity_init(UDF_INIT* initid, UDF_ARGS* args, char* message)
{
    if ((args->arg_count != 2) || (args->arg_type[0] != STRING_RESULT || args->arg_type[1] != STRING_RESULT)) {
        strcpy(message, "Function requires 2 arguments, (string, string)");
        return 1;
    }

    initid->ptr = NULL;
    initid->max_length = 6;
    initid->maybe_null = 0; //doesn't return null

    return 0;
}

double trgm_similarity(UDF_INIT* initid, UDF_ARGS* args, char* is_null, char* error)
{
    char* str1 = args->args[0];
    char* str2 = args->args[1];

    int len1 = (str1 == NULL) ? 0 : args->lengths[0];
    int len2 = (str2 == NULL) ? 0 : args->lengths[1];

    if (len1 == 0 || len2 == 0) {
        return 0.0;
    }
    return calc_similarity(str1, str2, len1, len2);
}
#else
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#define LPADDING 2
#define RPADDING 1
#define ISWORDCHAR(c) (isalpha((c)) || isdigit((c)))
#define IGNORECASE
#define CPTRGM(a, b)                               \
    do {                                           \
        *(((char*)(a)) + 0) = *(((char*)(b)) + 0); \
        *(((char*)(a)) + 1) = *(((char*)(b)) + 1); \
        *(((char*)(a)) + 2) = *(((char*)(b)) + 2); \
    } while (0);
#define CMPCHAR(a, b) (((a) == (b)) ? 0 : (((a) < (b)) ? -1 : 1))
#define CMPPCHAR(a, b, i) CMPCHAR(*(((const char*)(a)) + i), *(((const char*)(b)) + i))
#define CMPTRGM(a, b) (CMPPCHAR(a, b, 0) ? CMPPCHAR(a, b, 0) : (CMPPCHAR(a, b, 1) ? CMPPCHAR(a, b, 1) : CMPPCHAR(a, b, 2)))

typedef char trgm[3];

void debug_trgm(trgm* trg, int len)
{
    trgm* p_trgm = trg;
    while (p_trgm - trg < len) {
        char a = *(((char*)p_trgm) + 0);
        char b = *(((char*)p_trgm) + 1);
        char c = *(((char*)p_trgm) + 2);
        printf("%c%c%c ", a, b, c);
        p_trgm++;
    }
    printf("\n");
    fflush(stdout);
}

static int comp_trgm(const void* a, const void* b)
{
    return CMPTRGM(a, b);
}

static int unique_array(trgm* trg, int len)
{
    trgm *current_trgm, *tmp;
    current_trgm = tmp = trg;
    while (tmp - trg < len) {
        if (CMPTRGM(tmp, current_trgm)) {
            current_trgm++;
            CPTRGM(current_trgm, tmp);
            tmp++;
        } else {
            tmp++;
        }
    }
    return current_trgm - trg + 1;
}

/*
 * Finds first word in string, returns pointer to the word,
 * endword points to the character after word
 * From pg_trgm
 */
static char* find_word(char* str, int slen, char** endword, int* word_len)
{
    char* beginword = str;
    while (beginword - str < slen && !ISWORDCHAR(*beginword)) {
        beginword++;
    }
    if (beginword - str >= slen) {
        return NULL;
    }
    *endword = beginword;
    *word_len = 0;
    while (*endword - str < slen && ISWORDCHAR(**endword)) {
        (*endword)++;
        (*word_len)++;
    }
    return beginword;
}

char* lowerstr_with_len(char* begin_str, int len)
{
    char* output = (char*)malloc(sizeof(char) * (len + 1));
    char *p = begin_str, *out = output;
    while (p - begin_str < len && *p) {
        *(out++) = (char)tolower(*p++);
    }
    *out = '\0';
    return output;
}

trgm* make_trgms_from_word(trgm* p_trgm, char* str, int byte_len)
{
    char* p_str = str;
    if (byte_len < 3)
        return p_trgm;
    while (p_str - str < byte_len - 2) {
        CPTRGM(p_trgm, p_str);
        p_trgm++;
        p_str++;
    }
    return p_trgm;
}

int make_trgms(trgm* trg, char* str, int slen)
{
    trgm* p_trgm = trg;
    char* endword = str;
    char* beignword;
    int word_len;
    int bytelen;
    char* buf = (char*)malloc(slen + LPADDING + RPADDING + 1);
    if (LPADDING > 0) {
        *buf = ' ';
        if (LPADDING > 1) {
            *(buf + 1) = ' ';
        }
    }

    while ((beignword = find_word(endword, slen - (endword - str), &endword, &word_len)) != NULL) {
#ifdef IGNORECASE
        beignword = lowerstr_with_len(beignword, endword - beignword);
        bytelen = strlen(beignword);
#else
        bytelen = endword - beignword;
#endif
        memcpy(buf + LPADDING, beignword, bytelen);

#ifdef IGNORECASE
        free(beignword);
#endif
        buf[LPADDING + bytelen] = ' ';
        buf[LPADDING + bytelen + 1] = ' '; // ?

        p_trgm = make_trgms_from_word(p_trgm, buf, bytelen + LPADDING + RPADDING);
    }
    free(buf);
    return p_trgm - trg;
}

trgm* generate_trgm(char* str, int slen, int* trg_len)
{
    trgm* trg;
    int len;
    trg = (trgm*)malloc(sizeof(trgm) * (slen / 2 + 1) * 3);

    len = make_trgms(trg, str, slen);
    if (len == 0) {
        return trg;
    }
    *trg_len = len;
    if (len > 1) {
        qsort((void*)trg, len, sizeof(trgm), comp_trgm);
        unique_array(trg, len);
    }
    return trg;
}

double cnt_sml(trgm* trg1, trgm* trg2, int len1, int len2)
{
    trgm *p_trgm1 = trg1,
         *p_trgm2 = trg2;
    int count = 0;
    if (len1 <= 0 || len2 <= 0) {
        return 0.0;
    }
    while (p_trgm1 - trg1 < len1 && p_trgm2 - trg2 < len2) {
        int res = CMPTRGM(p_trgm1, p_trgm2);
        if (res < 0) {
            p_trgm1++;
        } else if (res > 0) {
            p_trgm2++;
        } else {
            p_trgm1++;
            p_trgm2++;
            count++;
        }
    }
    return (double)count / ((double)(len1 + len2 - count));
}

double calc_similarity(char* str1, char* str2, int len1, int len2)
{
    int trg_len1, trg_len2;
    trgm *trg1 = generate_trgm(str1, len1, &trg_len1),
         *trg2 = generate_trgm(str2, len2, &trg_len2);

    double res = cnt_sml(trg1, trg2, trg_len1, trg_len2);

    free(trg1);
    free(trg2);
    return res;
}

#ifdef STANDALONE
#define MAX_LENGTH 1000
int main()
{
    char s1[MAX_LENGTH + 1], s2[MAX_LENGTH + 1];
    fgets(s1, MAX_LENGTH, stdin);
    fgets(s2, MAX_LENGTH, stdin);
    printf("%lf\n", calc_similarity(s1, s2, strlen(s1), strlen(s2)));
}
#endif