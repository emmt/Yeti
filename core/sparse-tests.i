write, "*** TODO: check empty sparse matrix is possible";
write, "*** TODO: check scalar input/output spaces is possible";
write, "*** TODO: implement complex matrix";
write, "*** TODO: optimize (1) add coefficients with same indices";
write, "*** TODO: optimize (2) only keep non-zero coefficients (==> sparse_shrink becomes trivial)";
write, "*** TODO: optimize (3) sort coefficients according to transpose or not";

func sparse_test(row_dimlist, col_dimlist)
{
  a = random(row_dimlist, col_dimlist) - 0.5;
  dims = dimsof(a);
  a *= (random(dims) < 0.9); /* make it sparse */
  stride = numberof(array(char, row_dimlist));
  nonzero = where(a);
  s = sparse_matrix(a(nonzero),
                    row_dimlist, 1 + (nonzero - 1)%stride,
                    col_dimlist, 1 + (nonzero - 1)/stride);

  /* make A a 2-D matrix. */
  (ap = array(double, stride, numberof(a)/stride))(*) = a(*);

  x = random(col_dimlist);
  (y0 = array(double, row_dimlist))(*) = ap(,+)*(x(*))(+);
  y1 = s(x);
  y2 = mvmult(a, x);
  y3 = mvmult(s, x);
  write, format="y0 vs. y%d: %g\n", indgen(3),
    [max(abs(y1 - y0)), max(abs(y2 - y0)), max(abs(y3 - y0))];

  u = random(row_dimlist);
  (v0 = array(double, col_dimlist))(*) = ap(+,)*(u(*))(+);
  v1 = s(u, 1);
  v2 = mvmult(a, u, 1);
  v3 = mvmult(s, u, 1);
  write, format="v0 vs. v%d: %g\n", indgen(3),
    [max(abs(v1 - v0)), max(abs(v2 - v0)), max(abs(v3 - v0))];

  //error;

}
#if 1
sparse_test,[2,8,5],[3,11,2,3];
sparse_test,[3,8,5,11],[4,1,2,2,3];
#endif
