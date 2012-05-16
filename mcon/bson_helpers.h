#if PHP_C_BIGENDIAN
// reverse the bytes in an int
// wheeee stupid byte tricks
#define BYTE1_32(b) ((b & 0xff000000) >> 24)
#define BYTE2_32(b) ((b & 0x00ff0000) >> 8)
#define BYTE3_32(b) ((b & 0x0000ff00) << 8)
#define BYTE4_32(b) ((b & 0x000000ff) << 24)
#define MONGO_32(b) (BYTE4_32(b) | BYTE3_32(b) | BYTE2_32(b) | BYTE1_32(b))

#define BYTE1_64(b) ((b & 0xff00000000000000ll) >> 56)
#define BYTE2_64(b) ((b & 0x00ff000000000000ll) >> 40)
#define BYTE3_64(b) ((b & 0x0000ff0000000000ll) >> 24)
#define BYTE4_64(b) ((b & 0x000000ff00000000ll) >> 8)
#define BYTE5_64(b) ((b & 0x00000000ff000000ll) << 8)
#define BYTE6_64(b) ((b & 0x0000000000ff0000ll) << 24)
#define BYTE7_64(b) ((b & 0x000000000000ff00ll) << 40)
#define BYTE8_64(b) ((b & 0x00000000000000ffll) << 56)
#define MONGO_64(b) (BYTE8_64(b) | BYTE7_64(b) | BYTE6_64(b) | BYTE5_64(b) | BYTE4_64(b) | BYTE3_64(b) | BYTE2_64(b) | BYTE1_64(b))

#else
#define MONGO_32(b) (b)
#define MONGO_64(b) (b)
#endif

