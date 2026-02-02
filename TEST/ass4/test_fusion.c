void f(int *a, int *b, int n) {
  for (int i=0; i<n; i++) a[i] = a[i] + 1;
  for (int i=0; i<n; i++) b[i] = b[i] + 2;
}
