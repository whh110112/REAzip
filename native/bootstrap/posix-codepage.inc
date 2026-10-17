  // REAZIP_POSIX_CODEPAGE_V1: explicit legacy ZIP codepages use the OS iconv.
  // Upstream 26.04 forces UTF-8 on POSIX, even when a codepage was requested.
  if (codePage == 936 || codePage == 950 || codePage == 932)
  {
    const char *encoding = codePage == 936 ? "GBK" : (codePage == 950 ? "BIG5" : "SHIFT_JIS");
    iconv_t converter = iconv_open("UTF-8", encoding);
    if (converter == (iconv_t)-1)
      throw 282228;
    AString converted;
    const size_t capacity = (size_t)src.Len() * 4 + 16;
    char *output = converted.GetBuf((unsigned)capacity);
    char *input = const_cast<char *>(src.Ptr());
    size_t inputLeft = src.Len();
    size_t outputLeft = capacity;
    const size_t result = iconv(converter, &input, &inputLeft, &output, &outputLeft);
    iconv_close(converter);
    if (result == (size_t)-1 || inputLeft != 0)
      throw 282228;
    converted.ReleaseBuf_SetLen((unsigned)(capacity - outputLeft));
    if (!ConvertUTF8ToUnicode(converted, dest))
      throw 282228;
    return;
  }

