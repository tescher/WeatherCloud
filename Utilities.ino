// Byte array string compare

bool bComp(char* a1, char* a2) {
  for (int i = 0; ; i++) {
    if ((a1[i] == 0x00) && (a2[i] == 0x00)) return true;
    if (a1[i] != a2[i]) return false;
  }
}
