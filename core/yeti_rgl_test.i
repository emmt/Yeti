func rgl1(mu,x,g)
{
  if (! mu) return 0.0;
  r = (2.0*mu)*x(dif,..);
  penalty = (0.25/mu)*sum(r*r);
  if (! is_void(g)) {
    g(2:0,..) += r;
    g(1:-1,..) -= r;
  }
  return penalty;
}
func rgl2(mu,x,g)
{
  if (! mu) return 0.0;
  r = (2.0*mu)*x(,dif,..);
  penalty = (0.25/mu)*sum(r*r);
  if (! is_void(g)) {
    g(,2:0,..) += r;
    g(,1:-1,..) -= r;
  }
  return penalty;
}
func rgl3(mu,x,g)
{
  if (! mu) return 0.0;
  r = (2.0*mu)*x(,,dif,..);
  penalty = (0.25/mu)*sum(r*r);
  if (! is_void(g)) {
    g(,,2:0,..) += r;
    g(,,1:-1,..) -= r;
  }
  return penalty;
}
func rgl4(mu,x,g)
{
  if (! mu) return 0.0;
  r = (2.0*mu)*x(,,,dif,..);
  penalty = (0.25/mu)*sum(r*r);
  if (! is_void(g)) {
    g(,,,2:0,..) += r;
    g(,,,1:-1,..) -= r;
  }
  return penalty;
}
func rgl5(mu,x,g)
{
  if (! mu) return 0.0;
  r = (2.0*mu)*x(,,,,dif,..);
  penalty = (0.25/mu)*sum(r*r);
  if (! is_void(g)) {
    g(,,,,2:0,..) += r;
    g(,,,,1:-1,..) -= r;
  }
  return penalty;
}
func rgl6(mu,x,g)
{
  if (! mu) return 0.0;
  r = (2.0*mu)*x(,,,,,dif,..);
  penalty = (0.25/mu)*sum(r*r);
  if (! is_void(g)) {
    g(,,,,,2:0,..) += r;
    g(,,,,,1:-1,..) -= r;
  }
  return penalty;
}
/* periodic */
func rgl_p(mu,off,x,&g)
{
  if (mu == 0.0 || noneof(off)) {
    return 0.0;
  }
  dims = dimsof(x);
  ndims = dims(1);
  noffs = numberof(off);
  if (noffs > ndims) {
    if (anyof(off(ndims+1:noffs))) {
      error, "non-zero extra offset(s)";
    }
    off = off(1:ndims);
  } else if (noffs < ndims) {
    grow, off, array(long, ndims - noffs);
  }
  r = roll(x, off) - x;
  if (! is_void(g)) {
    g += (2.0*mu)*(roll(r, -off) - r);
  }
  return mu*sum(r*r);
}

func rgl1a(hyper,f,x,g)
{
  local grd;
  if (! hyper(1)) return 0.0;
  r = x(dif,..);
  if (is_void(g)) return f(hyper, r);
  penalty = f(hyper, r, grd);
  g(2:0,..) += grd;
  g(1:-1,..) -= grd;
  return penalty;
}
func rgl2a(hyper,f,x,g)
{
  local grd;
  if (! hyper(1)) return 0.0;
  r = x(,dif,..);
  if (is_void(g)) return f(hyper, r);
  penalty = f(hyper, r, grd);
  g(,2:0,..) += grd;
  g(,1:-1,..) -= grd;
  return penalty;
}
func rgl3a(hyper,f,x,g)
{
  local grd;
  if (! hyper(1)) return 0.0;
  r = x(,,dif,..);
  if (is_void(g)) return f(hyper, r);
  penalty = f(hyper, r, grd);
  g(,,2:0,..) += grd;
  g(,,1:-1,..) -= grd;
  return penalty;
}
func rgl4a(hyper,f,x,g)
{
  local grd;
  if (! hyper(1)) return 0.0;
  r = x(,,,dif,..);
  if (is_void(g)) return f(hyper, r);
  penalty = f(hyper, r, grd);
  g(,,,2:0,..) += grd;
  g(,,,1:-1,..) -= grd;
  return penalty;
}
func rgl5a(hyper,f,x,g)
{
  local grd;
  if (! hyper(1)) return 0.0;
  r = x(,,,,dif,..);
  if (is_void(g)) return f(hyper, r);
  penalty = f(hyper, r, grd);
  g(,,,,2:0,..) += grd;
  g(,,,,1:-1,..) -= grd;
  return penalty;
}

func rgl_test
{
  format = "%2d %-15s - delta_penalty = %9.2g / max(|delta_gradient|) = %g\n";
  mu = pi;
  eps = 0.1;
  x = random(3,4,5,6,7) - 0.5;

  /*----------------------------------------*/
  cost = "l2";

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl1(mu,x,g0);
  e1 = rgl_roughness_l2(mu,1,x,g1);
  write, format=format, 1, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl2(mu,x,g0);
  e1 = rgl_roughness_l2(mu,[0,1],x,g1);
  write, format=format, 2, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl3(mu,x,g0);
  e1 = rgl_roughness_l2(mu,[0,0,1],x,g1);
  write, format=format, 3, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl4(mu,x,g0);
  e1 = rgl_roughness_l2(mu,[0,0,0,1],x,g1);
  write, format=format, 4, cost, e1 - e0, max(abs(g1 - g0));

  /*----------------------------------------*/
  cost = "l2_periodic";

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  off = 1;
  e0 = rgl_p(mu,off,x,g0);
  e1 = rgl_roughness_l2_periodic(mu,off,x,g1);
  write, format=format, 5, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  off = [0,1];
  e0 = rgl_p(mu,off,x,g0);
  e1 = rgl_roughness_l2_periodic(mu,off,x,g1);
  write, format=format, 6, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  off = [0,0,1];
  e0 = rgl_p(mu,off,x,g0);
  e1 = rgl_roughness_l2_periodic(mu,off,x,g1);
  write, format=format, 7, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  off = [0,1,1];
  e0 = rgl_p(mu,off,x,g0);
  e1 = rgl_roughness_l2_periodic(mu,off,x,g1);
  write, format=format, 8, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  off = [0,2,0,1];
  e0 = rgl_p(mu,off,x,g0);
  e1 = rgl_roughness_l2_periodic(mu,off,x,g1);
  write, format=format, 9, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  off = [0,2,0,-3];
  e0 = rgl_p(mu,off,x,g0);
  e1 = rgl_roughness_l2_periodic(mu,off,x,g1);
  write, format=format, 10, cost, e1 - e0, max(abs(g1 - g0));

  /*----------------------------------------*/
  cost = "l2l1";
  hyper = [mu, eps];

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl1a(hyper,cost_l2l1,x,g0);
  e1 = rgl_roughness_l2l1(hyper,1,x,g1);
  write, format=format, 11, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl2a(hyper,cost_l2l1,x,g0);
  e1 = rgl_roughness_l2l1(hyper,[0,1],x,g1);
  write, format=format, 12, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl3a(hyper,cost_l2l1,x,g0);
  e1 = rgl_roughness_l2l1(hyper,[0,0,1],x,g1);
  write, format=format, 13, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl4a(hyper,cost_l2l1,x,g0);
  e1 = rgl_roughness_l2l1(hyper,[0,0,0,1],x,g1);
  write, format=format, 14, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl5a(hyper,cost_l2l1,x,g0);
  e1 = rgl_roughness_l2l1(hyper,[0,0,0,0,1],x,g1);
  write, format=format, 15, cost, e1 - e0, max(abs(g1 - g0));

  /*----------------------------------------*/
  cost = "l2l0";
  hyper = [mu, eps];

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl1a(hyper,cost_l2l0,x,g0);
  e1 = rgl_roughness_l2l0(hyper,1,x,g1);
  write, format=format, 16, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl2a(hyper,cost_l2l0,x,g0);
  e1 = rgl_roughness_l2l0(hyper,[0,1],x,g1);
  write, format=format, 17, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl3a(hyper,cost_l2l0,x,g0);
  e1 = rgl_roughness_l2l0(hyper,[0,0,1],x,g1);
  write, format=format, 18, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl4a(hyper,cost_l2l0,x,g0);
  e1 = rgl_roughness_l2l0(hyper,[0,0,0,1],x,g1);
  write, format=format, 19, cost, e1 - e0, max(abs(g1 - g0));

  g0 = array(double, dimsof(x));
  g1 = array(double, dimsof(x));
  e0 = rgl5a(hyper,cost_l2l0,x,g0);
  e1 = rgl_roughness_l2l0(hyper,[0,0,0,0,1],x,g1);
  write, format=format, 20, cost, e1 - e0, max(abs(g1 - g0));
}

plug_dir,".";
include,"./yeti.i";
rgl_test;
