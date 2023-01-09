//clang.exe -o decoderC.exe .\decoderC.c -I .\zlib -l .\zlib\zlibwapi -L .\zlib 
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "zlib\zlib.h"

#define PNG_SIGNATURE 0xa1a0a0d474e5089

typedef unsigned char byte;

struct chunk {
    char type[4];
    unsigned int length;
    unsigned char *data;
    unsigned int crc;
};

//! Byte swap unsigned int
uint32_t swap_uint32( uint32_t val )
{
    val = ((val << 8) & 0xFF00FF00 ) | ((val >> 8) & 0xFF00FF ); 
    return (val << 16) | (val >> 16);
}

void read_chunk(FILE *f, struct chunk *chunk) {
    if (fread(&chunk->length, 4, 1, f) != 1 ||
        fread(chunk->type, 1, 4, f) != 4) {
        printf("Error reading chunk header\n");
        exit(EXIT_FAILURE);
    }
    chunk->length = swap_uint32(chunk->length);

    
    chunk->data = malloc(chunk->length);
    if (fread(chunk->data, 1, chunk->length, f) != chunk->length) {
        printf("Error reading chunk data\n");
        exit(EXIT_FAILURE);
    }
    printf("%i\n", chunk->length);
    if (fread(&chunk->crc, 4, 1, f) != 1) {
        printf("Error reading chunk checksum\n");
        exit(EXIT_FAILURE);
    }
    chunk->crc = swap_uint32(chunk->crc);

    unsigned int checksum = crc32(0L, Z_NULL, 0);
    checksum = crc32(checksum, (unsigned char *)chunk->type, 4);
    
    checksum = crc32(checksum, chunk->data, chunk->length);

    if (chunk->crc != checksum) {
        printf("chunk checksum failed %u != %u\n", chunk->crc, checksum);
        exit(EXIT_FAILURE);
    }
}


int main(int argc, char* argv[]) {
    FILE *f = fopen("basn6a08.png", "rb");

    size_t stream;

    //TODO
    if (fread(&stream, 1, sizeof(size_t), f) != sizeof(size_t) 
    // || strncmp(stream, PNG_SIGNATURE, sizeof(size_t)) != 0
    ) {
        printf("Invalid PNG Signature\n");
        exit(EXIT_FAILURE);
    }

    struct chunk chunk;
    struct chunk *chunks = NULL;
    size_t num_chunks = 0;

    while (1) {
        read_chunk(f, &chunk);
        chunks = realloc(chunks, (num_chunks + 1) * sizeof(struct chunk));
        memcpy(&chunks[num_chunks], &chunk, sizeof(struct chunk));
        num_chunks++;

        if (strncmp(chunk.type, "IEND", 4) == 0) {
            break;
        }
    }

    for (size_t i = 0; i < num_chunks; i++) {
        printf("%.4s\n", chunks[i].type);
    }

    unsigned int width, height, bitd, colort, compm, filterm, interlacem;
    
    memcpy(&width, chunks[0].data, 4);
    width = swap_uint32(width);
    printf("%u\n",width);

    memcpy(&height, &chunks[0].data[4], 4);
    height = swap_uint32(height);
    printf("%u\n", height);

    bitd = chunks[0].data[8];
    colort = chunks[0].data[9];
    compm = chunks[0].data[10];
    filterm = chunks[0].data[11];
    interlacem = chunks[0].data[12];

    //TODO if (sscanf((char *)chunks[0].data, "%u%u%u%u%u%u%u", &width, &height, &bitd, &colort, &compm, &filterm, &interlacem) != 7) {
    //     printf("Error parsing IHDR chunk data\n");
    //     exit(EXIT_FAILURE);
    // }
    if (compm != 0) {
        printf("invalid compression method\n");
        exit(EXIT_FAILURE);
    }
    if (filterm != 0) {
        printf("invalid filter method\n");
        exit(EXIT_FAILURE);
    }
    if (colort != 6) {
        printf("we only support truecolor with alpha\n");
        exit(EXIT_FAILURE);
    }
    if (bitd != 8) {
        printf("we only support a bit depth of 8\n");
        exit(EXIT_FAILURE);
    }
    if (interlacem != 0) {
        printf("we only support no interlacing\n");
        exit(EXIT_FAILURE);
    }



    size_t IDAT_size = 0;
    for (size_t i = 0; i < num_chunks; i++) {
        if (strncmp(chunks[i].type, "IDAT", 4) == 0) {
            IDAT_size += chunks[i].length;
        }
    }

    unsigned char *IDAT_data = malloc(IDAT_size);
    size_t IDAT_offset = 0;
    for (size_t i = 0; i < num_chunks; i++) {
        if (strncmp(chunks[i].type, "IDAT", 4) == 0) {
            memcpy(IDAT_data + IDAT_offset, chunks[i].data, chunks[i].length);
            IDAT_offset += chunks[i].length;
        }
    }
    printf("%s\n", IDAT_data);
    unsigned long decompressed_size = 0;
    unsigned char *decompressed_data = NULL;
    int ret = uncompress(decompressed_data, &decompressed_size, IDAT_data, IDAT_size);
    if (ret != Z_OK) {
        fprintf(stderr, "Error decompressing IDAT data: %d\n", ret);
        exit(EXIT_FAILURE);
    }

    printf("%lu\n", decompressed_size);


    fclose(f);
    for (size_t i = 0; i < num_chunks; i++) {
        free(chunks[i].data);
    }
    free(chunks);
    free(decompressed_data);
    free(IDAT_data);
    return 0;
}