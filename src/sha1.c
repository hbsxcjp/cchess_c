/*
* sha1.c
*
* Description:
* This file implements the Secure Hashing Algorithm 1 as
* defined in FIPS PUB 180-1 published April 17, 1995.
*
* The SHA-1, produces a 160-bit message digest for a given
* data stream. It should take about 2**n steps to find a
* message with the same digest as a given message and
* 2**(n/2) to find any two messages with the same digest,
* when n is the digest size in bits. Therefore, this
* algorithm can serve as a means of providing a
* "fingerprint" for a message.
*
* Portability Issues:
* SHA-1 is defined in terms of 32-bit "words". This code
* uses <stdint.h> (included via "sha1.h" to define 32 and 8
* bit unsigned integer types. If your C compiler does not
* support 32 bit unsigned integers, this code is not
* appropriate.
*
* Caveats:
* SHA-1 is designed to work with messages less than 2^64 bits
* long. Although SHA-1 allows a message digest to be generated
* for messages of any number of bits less than 2^64, this
* implementation only works with messages with a length that is
* a multiple of the size of an 8-bit character.

*描述：
*该文件实现了1995年4月17日发布的FIPS PUB 180-1中定义的安全哈希算法1。
*
* SHA-1为给定的数据流生成一个160位的消息摘要。当n是摘要大小（以位为单位）时，
大约需要2 ** n个步骤来查找与给定消息具有相同摘要的消息，而需要2 **（n / 2）
个步骤来查找具有相同摘要的任何两个消息。因此，该算法可以用作为消息提供“指纹”的手段。
*
*可移植性问题：SHA-1是根据32位“字”定义的。该代码使用<stdint.h>
（通过“ sha1.h”包含）来定义32位和8位无符号整数类型。如果您的C编译器不支持32位无符号整数，则此代码不合适。
*
注意事项：SHA-1设计用于处理长度小于2 ^ 64位的消息。尽管SHA-1允许为少于
2 ^ 64的任何位数的消息生成消息摘要，但是此实现仅适用于长度为8位字符大小的倍数的消息。
*/

#include "head/sha1.h"
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
* Define the SHA1 circular left shift macro
*/
#define SHA1CircularShift(bits, word) \
    (((word) << (bits)) | ((word) >> (32 - (bits))))
/* Local Function Prototyptes */
void SHA1PadMessage(SHA1Context*);
void SHA1ProcessMessageBlock(SHA1Context*);
/*
* SHA1Reset
*
* Description:
* This function will initialize the SHA1Context in preparation
* for computing a new SHA1 message digest.
* 此函数将初始化SHA1Context，以准备计算新的SHA1消息摘要。

* Parameters:
* context: [in/out]
* The context to reset.
*
* Returns:
* sha Error Code.
*
*/
int SHA1Reset(SHA1Context* context) //初始化状态
{
    if (!context) {
        return shaNull;
    }
    context->Length_Low = 0;
    context->Length_High = 0;
    context->Message_Block_Index = 0;
    context->Intermediate_Hash[0] = 0x67452301; //取得的HASH结果（中间数据）
    context->Intermediate_Hash[1] = 0xEFCDAB89;
    context->Intermediate_Hash[2] = 0x98BADCFE;
    context->Intermediate_Hash[3] = 0x10325476;
    context->Intermediate_Hash[4] = 0xC3D2E1F0;
    context->Computed = 0;
    context->Corrupted = 0;
    return shaSuccess;
}

/*
* SHA1Result
*
* Description:
* This function will return the 160-bit message digest into the
* Message_Digest array provided by the caller.
* NOTE: The first octet of hash is stored in the 0th element,
* the last octet of hash in the 19th element.
 此函数会将160位消息摘要返回到调用方提供的Message_Digest数组中。 
 注意：哈希的第一个八位位组存储在第0个元素中，哈希的最后一个八位位组存储在第19个元素中。
*
* Parameters:
* context: [in/out]
* The context to use to calculate the SHA-1 hash.
* Message_Digest: [out]
* Where the digest is returned.
*
* Returns:
* sha Error Code.
*
*/
int SHA1Result(SHA1Context* context, uint8_t Message_Digest[SHA1HashSize])
{
    int i;
    if (!context || !Message_Digest) {
        return shaNull;
    }
    if (context->Corrupted) {
        return context->Corrupted;
    }
    if (!context->Computed) {
        SHA1PadMessage(context);
        for (i = 0; i < 64; ++i) {
            /* message may be sensitive, clear it out */
            context->Message_Block[i] = 0;
        }
        context->Length_Low = 0; /* and clear length */
        context->Length_High = 0;
        context->Computed = 1;
    }
    for (i = 0; i < SHA1HashSize; ++i) {
        Message_Digest[i] = context->Intermediate_Hash[i >> 2] >> 8 * (3 - (i & 0x03));
    }
    return shaSuccess;
}

/*
* SHA1Input
*
* Description:
* This function accepts an array of octets as the next portion
* of the message. 
 此函数接受八位字节数组作为消息的下一部分。
*
* Parameters:
* context: [in/out]
* The SHA context to update
* message_array: [in]
* An array of characters representing the next portion of 代表消息下一部分的字符数组。
 
* the message.
* length: [in]
* The length of the message in message_array
*
* Returns:
* sha Error Code.
*
*/

int SHA1Input(SHA1Context* context, const uint8_t* message_array, unsigned length)
{
    if (!length) {
        return shaSuccess;
    }
    if (!context || !message_array) {
        return shaNull;
    }
    if (context->Computed) {
        context->Corrupted = shaStateError;
        return shaStateError;
    }
    if (context->Corrupted) {
        return context->Corrupted;
    }
    while (length-- && !context->Corrupted) {
        context->Message_Block[context->Message_Block_Index++] = (*message_array & 0xFF);
        context->Length_Low += 8;
        if (context->Length_Low == 0) {
            context->Length_High++;
            if (context->Length_High == 0) {
                /* Message is too long */
                context->Corrupted = 1;
            }
        }
        if (context->Message_Block_Index == 64) {
            SHA1ProcessMessageBlock(context);
        }
        message_array++;
    }
    return shaSuccess;
}

/*
* SHA1ProcessMessageBlock
*
* Description:
* This function will process the next 512 bits of the message
* stored in the Message_Block array. 
 此函数将处理存储在Message_Block数组中的消息的后512位。
*
* Parameters:
* None.
*
* Returns:
* Nothing.
*
* Comments:
* Many of the variable names in this code, especially the
* single character names, were used because those were the
* names used in the publication.
*
*/

void SHA1ProcessMessageBlock(SHA1Context* context)
{
    const uint32_t K[] = { /* Constants defined in SHA-1 */
        0x5A827999,
        0x6ED9EBA1,
        0x8F1BBCDC,
        0xCA62C1D6
    };
    int t; /* Loop counter */
    uint32_t temp; /* Temporary word value */
    uint32_t W[80]; /* Word sequence */
    uint32_t A, B, C, D, E; /* Word buffers */
    /*
	* Initialize the first 16 words in the array W
	*/
    for (t = 0; t < 16; t++) {
        W[t] = context->Message_Block[t * 4] << 24;
        W[t] |= context->Message_Block[t * 4 + 1] << 16;
        W[t] |= context->Message_Block[t * 4 + 2] << 8;
        W[t] |= context->Message_Block[t * 4 + 3];
    }
    for (t = 16; t < 80; t++) {
        W[t] = SHA1CircularShift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
    }
    A = context->Intermediate_Hash[0];
    B = context->Intermediate_Hash[1];
    C = context->Intermediate_Hash[2];
    D = context->Intermediate_Hash[3];
    E = context->Intermediate_Hash[4];
    for (t = 0; t < 20; t++) {
        temp = SHA1CircularShift(5, A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
        E = D;
        D = C;
        C = SHA1CircularShift(30, B);
        B = A;
        A = temp;
    }
    for (t = 20; t < 40; t++) {
        temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
        E = D;
        D = C;
        C = SHA1CircularShift(30, B);
        B = A;
        A = temp;
    }
    for (t = 40; t < 60; t++) {
        temp = SHA1CircularShift(5, A) + ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
        E = D;
        D = C;
        C = SHA1CircularShift(30, B);
        B = A;
        A = temp;
    }
    for (t = 60; t < 80; t++) {
        temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
        E = D;
        D = C;
        C = SHA1CircularShift(30, B);
        B = A;
        A = temp;
    }
    context->Intermediate_Hash[0] += A;
    context->Intermediate_Hash[1] += B;
    context->Intermediate_Hash[2] += C;
    context->Intermediate_Hash[3] += D;
    context->Intermediate_Hash[4] += E;
    context->Message_Block_Index = 0;
}

/*
* SHA1PadMessage
*
* Description:
* According to the standard, the message must be padded to an even
* 512 bits. The first padding bit must be a ’1’. The last 64
* bits represent the length of the original message. All bits in
* between should be 0. This function will pad the message
* according to those rules by filling the Message_Block array
* accordingly. It will also call the ProcessMessageBlock function
* provided appropriately. When it returns, it can be assumed that
* the message digest has been computed.
 根据标准，消息必须填充为偶数512位。 第一个填充位必须为“ 1”。 后64位代表原始消息的长度。 
 两者之间的所有位都应为0。此函数将通过填充相应的Message_Block数组，根据这些规则填充消息。 
 它还将调用适当提供的ProcessMessageBlock函数。 返回时，可以假定已计算出消息摘要。
*
* Parameters:
* context: [in/out]
* The context to pad
* ProcessMessageBlock: [in]
* The appropriate SHA*ProcessMessageBlock function
* Returns:
* Nothing.
*
*/

void SHA1PadMessage(SHA1Context* context)
{
    /*
	* Check to see if the current message block is too small to hold
	* the initial padding bits and length. If so, we will pad the
	* block, process it, and then continue padding into a second
	* block.
      检查当前消息块是否太小而无法容纳初始填充位和长度。 
      如果是这样，我们将填充该块，对其进行处理，然后继续填充到第二个块中。
	*/
    if (context->Message_Block_Index > 55) {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while (context->Message_Block_Index < 64) {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
        SHA1ProcessMessageBlock(context);
        while (context->Message_Block_Index < 56) {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    } else {
        context->Message_Block[context->Message_Block_Index++] = 0x80;
        while (context->Message_Block_Index < 56) {
            context->Message_Block[context->Message_Block_Index++] = 0;
        }
    }

    /*
	* Store the message length as the last 8 octets
     将消息长度存储为最后8个八位位组
	*/
    context->Message_Block[56] = context->Length_High >> 24;
    context->Message_Block[57] = context->Length_High >> 16;
    context->Message_Block[58] = context->Length_High >> 8;
    context->Message_Block[59] = context->Length_High;
    context->Message_Block[60] = context->Length_Low >> 24;
    context->Message_Block[61] = context->Length_Low >> 16;
    context->Message_Block[62] = context->Length_Low >> 8;
    context->Message_Block[63] = context->Length_Low;
    SHA1ProcessMessageBlock(context);
}

/*
* sha1test.c
*
* Description:
* This file will exercise the SHA-1 code performing the three
* tests documented in FIPS PUB 180-1 plus one which calls
* SHA1Input with an exact multiple of 512 bits, plus a few
* error test checks.
 该文件将执行SHA-1代码，执行FIPS PUB 180-1中记录的三个测试，
 再加上一个以512位精确倍数调用SHA1Input的代码，再进行一些错误测试检查。
*
* Portability Issues:
* None.
*
*/

#include "head/sha1.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/*
* Define patterns for testing
*/
#define TEST1 "abc"
#define TEST2a "abcdbcdecdefdefgefghfghighijhi"

#define TEST2b "jkijkljklmklmnlmnomnopnopq"
#define TEST2 TEST2a TEST2b
#define TEST3 "a"
#define TEST4a "01234567012345670123456701234567"
#define TEST4b "01234567012345670123456701234567"
/* an exact multiple of 512 bits */
#define TEST4 TEST4a TEST4b
char* testarray[4] = {
    TEST1,
    TEST2,
    TEST3,
    TEST4
};

long int repeatcount[4] = { 1, 1, 1000000, 10 };
char* resultarray[4] = {
    "A9 99 3E 36 47 06 81 6A BA 3E 25 71 78 50 C2 6C 9C D0 D8 9D",
    "84 98 3E 44 1C 3B D2 6E BA AE 4A A1 F9 51 29 E5 E5 46 70 F1",
    "34 AA 97 3C D4 C4 DA A4 F6 1E EB 2B DB AD 27 31 65 34 01 6F",
    "DE A3 56 A2 CD DD 90 C7 A7 EC ED C5 EB B5 63 93 4F 46 04 52"
};

void ustrToSHA1(unsigned char* sha1, const unsigned char* source)
{
    SHA1Context context;
    SHA1Reset(&context);
    SHA1Input(&context, source, strlen((char*)source));
    SHA1Result(&context, sha1);
}

int testsha1(void)
{
    SHA1Context sha;
    int i, j, err;
    uint8_t Message_Digest[20];
    /*
	* Perform SHA-1 tests
	*/
    for (j = 0; j < 4; ++j) {
        //printf( "\nTest %d: %ld, ’%s’\n",j+1,repeatcount[j],testarray[j]);
        err = SHA1Reset(&sha);
        //if (err)
        //{
        //fprintf(stderr, "SHA1Reset Error %d.\n", err );
        //break; /* out of for j loop */
        //}
        for (i = 0; i < repeatcount[j]; ++i) {
            err = SHA1Input(&sha,
                (const unsigned char*)testarray[j],
                strlen(testarray[j]));
            if (err) {
                fprintf(stderr, "SHA1Input Error %d.\n", err);
                break; /* out of for i loop */
            }
        }
        err = SHA1Result(&sha, Message_Digest);
        if (err) {
            fprintf(stderr,
                "SHA1Result Error %d, could not compute message digest.\n",
                err);
        } else {
            printf("\t");
            for (i = 0; i < 20; ++i) {
                printf("%02X ", Message_Digest[i]);
            }
            printf("\n");
        }
        printf("Should match:\n");
        printf("\t%s\n\n", resultarray[j]);
    }

    /* Test some error returns */
    err = SHA1Input(&sha, (const unsigned char*)testarray[1], 1);
    printf("\nError %d. Should be %d.\n", err, shaStateError);
    err = SHA1Reset(0);
    printf("\nError %d. Should be %d.\n", err, shaNull);
    return 0;
}

#ifdef __cplusplus
}
#endif