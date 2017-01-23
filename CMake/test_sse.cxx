#include <emmintrin.h>
int main()
{
  double a[]  = { 25.3, 3.64 };
  double b[]  = { 32.5, 15.6 };
  double result[2] = {0.0};

  __m128d z = _mm_mul_pd(_mm_loadu_pd(a),_mm_loadu_pd(b));

  _mm_storeu_pd(result,z);

  return 0;
}
