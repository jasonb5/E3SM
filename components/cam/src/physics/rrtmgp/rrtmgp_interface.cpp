
#include <iostream>
#include <iomanip>
#include <chrono>
#include <cstdlib>
#include <vector>
#include "mo_rrtmgp_util_string.h"
#include "mo_gas_concentrations.h"
#include "mo_gas_optics_rrtmgp.h"
#include "const_rrtmgpxx.h"
#include "mo_load_coefficients.h"
#include "mo_fluxes.h"
#include "mo_fluxes_byband.h"
#include "mo_rte_lw.h"
#include "mo_rte_sw.h"


std::vector<std::string> gas_names_vec;
string1d gas_names;
GasOpticsRRTMGP k_dist_lw;
GasOpticsRRTMGP k_dist_sw;


extern "C" void clear_gas_names() {
  gas_names_vec = std::vector<std::string>();
  gas_names = string1d();
}


extern "C" void push_gas_name(char *gas_name) {
  gas_names_vec.push_back(lower_case(std::string(gas_name)));
}


extern "C" void gas_names_to_string1d() {
  int ngas = gas_names_vec.size();
  gas_names = string1d("gas_names",ngas);
  for (int i=1; i <= ngas; i++) {
    gas_names(i) = gas_names_vec[i-1];
  }
}


// TODO: create string1d "gas_names" before calling this function
// TODO: Make a fortran wrapper to transform character arrays into c-style
extern "C" void rrtmgp_initialize_cpp(char *coefficients_file_sw, char *coefficients_file_lw) {
  GasConcs gas_concentrations;
  // PCOLS and PLEV don't actually matter here. This GasConcs object is only telling us which gas names are available
  // There is no set_vmr or get_vmr called on the object called gas_names in this scope.
  gas_concentrations.init(gas_names , PCOLS , PLEV+1);
  load_and_init(k_dist_sw , coefficients_file_sw , gas_concentrations);
  load_and_init(k_dist_lw , coefficients_file_lw , gas_concentrations);
}


extern "C" int rrtmgp_finalize() {
  gas_names_vec = std::vector<std::string>();
  gas_names = string1d();
  k_dist_lw.finalize();
  k_dist_sw.finalize();
  return 0;
}


extern "C" int get_nband_sw() {
  return k_dist_sw.get_nband();
}


extern "C" int get_nband_lw() {
  return k_dist_lw.get_nband();
}


extern "C" int get_ngpt_sw() {
  return k_dist_sw.get_ngpt();
}


extern "C" int get_ngpt_lw() {
  return k_dist_lw.get_ngpt();
}


extern "C" void get_band_lims_wavenumber_sw(int nbnd, real *band_limits_p) {
  realHost2d band_limits("band_limits",band_limits_p,2,nbnd);
  k_dist_sw.get_band_lims_wavenumber().deep_copy_to(band_limits);
}


extern "C" void get_band_lims_wavenumber_lw(int nbnd, real *band_limits_p) {
  realHost2d band_limits("band_limits",band_limits_p,2,nbnd);
  k_dist_lw.get_band_lims_wavenumber().deep_copy_to(band_limits);
}


extern "C" real get_temp_min() { return min(k_dist_sw.get_temp_min() , k_dist_lw.get_temp_min()); }


extern "C" real get_temp_max() { return max(k_dist_sw.get_temp_max() , k_dist_lw.get_temp_max()); }


extern "C" void get_gpoint_bands_sw(int ngpt, int *gpoint_bands_p) {
  intHost1d gpoint_bands("gpoint_bands",gpoint_bands_p,ngpt);
  k_dist_sw.get_gpoint_bands().deep_copy_to(gpoint_bands);
}


extern "C" void get_gpoint_bands_lw(int ngpt, int *gpoint_bands_p) {
  intHost1d gpoint_bands("gpoint_bands",gpoint_bands_p,ngpt);
  k_dist_lw.get_gpoint_bands().deep_copy_to(gpoint_bands);
}


extern "C" void get_band_midpoints_sw(int nband, real *band_midpoints_p) {
  realHost1d band_midpoints("band_midpoints",band_midpoints_p,nband);
  realHost2d band_limits("band_limits",2,nband);

  // Get band limits
  k_dist_sw.get_band_lims_wavelength().deep_copy_to(band_limits);

  // Compute midpoints from band limits
  memset( band_midpoints , 0._wp );
  for (int i=1 ; i <= nband ; i++) {
    band_midpoints(i) = (band_limits(1,i) + band_limits(2,i)) / 2._wp;
  };
}


extern "C" void get_band_midpoints_lw(int nband, real *band_midpoints_p) {
  realHost1d band_midpoints("band_midpoints",band_midpoints_p,nband);
  realHost2d band_limits("band_limits",2,nband);

  // Get band limits
  k_dist_lw.get_band_lims_wavelength().deep_copy_to(band_limits);

  // Compute midpoints from band limits
  memset( band_midpoints , 0._wp );
  for (int i=1 ; i <= nband ; i++) {
    band_midpoints(i) = (band_limits(1,i) + band_limits(2,i)) / 2._wp;
  };
}


// TODO: create string1d "gas_names" before calling this function
extern "C" void rrtmgp_run_lw_cpp(int ngas, int ncol, int nlay, int nbnd, int ngpt, real *gas_vmr_p, real *p_lay_p,
                                  real *t_lay_p, real *p_lev_p, real *t_sfc_p, real *sfc_emis_p, real *cld_tau_p,
                                  real *aer_tau_p, real *allsky_flux_up_p, real *allsky_flux_dn_p, real *allsky_flux_net_p,
                                  real *allsky_bnd_flux_up_p, real *allsky_bnd_flux_dn_p, real *allsky_bnd_flux_net_p,
                                  real *clrsky_flux_up_p, real *clrsky_flux_dn_p, real *clrsky_flux_net_p,
                                  real *clrsky_bnd_flux_up_p, real *clrsky_bnd_flux_dn_p, real *clrsky_bnd_flux_net_p, real *t_lev_p,
                                  int n_gauss_angles) {
  // Wrap pointers in YAKL Arrays
  real3d gas_vmr             = realHost3d("gas_vmr            ",gas_vmr_p            ,ngas,ncol,nlay  ).createDeviceCopy();       
  real2d p_lay               = realHost2d("p_lay              ",p_lay_p              ,ncol,nlay       ).createDeviceCopy();       
  real2d t_lay               = realHost2d("t_lay              ",t_lay_p              ,ncol,nlay       ).createDeviceCopy();       
  real2d p_lev               = realHost2d("p_lev              ",p_lev_p              ,ncol,nlay+1     ).createDeviceCopy();       
  real1d t_sfc               = realHost1d("t_sfc              ",t_sfc_p              ,ncol            ).createDeviceCopy();       
  real2d sfc_emis            = realHost2d("sfc_emis           ",sfc_emis_p           ,nbnd,ncol       ).createDeviceCopy();       
  real3d cld_tau             = realHost3d("cld_tau            ",cld_tau_p            ,ncol,nlay,ngpt  ).createDeviceCopy();       
  real3d aer_tau             = realHost3d("aer_tau            ",aer_tau_p            ,ncol,nlay,nbnd  ).createDeviceCopy();       
  real2d allsky_flux_up      = realHost2d("allsky_flux_up     ",allsky_flux_up_p     ,ncol,nlay+1     ).createDeviceCopy();       
  real2d allsky_flux_dn      = realHost2d("allsky_flux_dn     ",allsky_flux_dn_p     ,ncol,nlay+1     ).createDeviceCopy();       
  real2d allsky_flux_net     = realHost2d("allsky_flux_net    ",allsky_flux_net_p    ,ncol,nlay+1     ).createDeviceCopy();       
  real2d clrsky_flux_up      = realHost2d("clrsky_flux_up     ",clrsky_flux_up_p     ,ncol,nlay+1     ).createDeviceCopy();       
  real2d clrsky_flux_dn      = realHost2d("clrsky_flux_dn     ",clrsky_flux_dn_p     ,ncol,nlay+1     ).createDeviceCopy();       
  real2d clrsky_flux_net     = realHost2d("clrsky_flux_net    ",clrsky_flux_net_p    ,ncol,nlay+1     ).createDeviceCopy();       
  real3d allsky_bnd_flux_up  = realHost3d("allsky_bnd_flux_up ",allsky_bnd_flux_up_p ,ncol,nlay+1,nbnd).createDeviceCopy();       
  real3d allsky_bnd_flux_dn  = realHost3d("allsky_bnd_flux_dn ",allsky_bnd_flux_dn_p ,ncol,nlay+1,nbnd).createDeviceCopy();       
  real3d allsky_bnd_flux_net = realHost3d("allsky_bnd_flux_net",allsky_bnd_flux_net_p,ncol,nlay+1,nbnd).createDeviceCopy();       
  real3d clrsky_bnd_flux_up  = realHost3d("clrsky_bnd_flux_up ",clrsky_bnd_flux_up_p ,ncol,nlay+1,nbnd).createDeviceCopy();       
  real3d clrsky_bnd_flux_dn  = realHost3d("clrsky_bnd_flux_dn ",clrsky_bnd_flux_dn_p ,ncol,nlay+1,nbnd).createDeviceCopy();       
  real3d clrsky_bnd_flux_net = realHost3d("clrsky_bnd_flux_net",clrsky_bnd_flux_net_p,ncol,nlay+1,nbnd).createDeviceCopy();       
  real2d t_lev               = realHost2d("t_lev              ",t_lev_p              ,ncol,nlay+1     ).createDeviceCopy();

  // Flag for TOA->SFC or SFC->TOA
  auto p_lay_host = p_lay.createHostCopy();
  bool top_at_1 = p_lay_host(1,1) < p_lay_host(1,nlay);

  // Assign Arrays to the fluxes objects
  FluxesByband allsky_fluxes;
  allsky_fluxes.flux_up      = allsky_flux_up;
  allsky_fluxes.flux_dn      = allsky_flux_dn;
  allsky_fluxes.flux_net     = allsky_flux_net;
  allsky_fluxes.bnd_flux_up  = allsky_bnd_flux_up;
  allsky_fluxes.bnd_flux_dn  = allsky_bnd_flux_dn;
  allsky_fluxes.bnd_flux_net = allsky_bnd_flux_net;

  FluxesByband clrsky_fluxes;
  clrsky_fluxes.flux_up      = clrsky_flux_up;
  clrsky_fluxes.flux_dn      = clrsky_flux_dn;
  clrsky_fluxes.flux_net     = clrsky_flux_net;
  clrsky_fluxes.bnd_flux_up  = clrsky_bnd_flux_up;
  clrsky_fluxes.bnd_flux_dn  = clrsky_bnd_flux_dn;
  clrsky_fluxes.bnd_flux_net = clrsky_bnd_flux_net;

  // Setup gas concentrations
  GasConcs gas_concs;
  gas_concs.init(gas_names,ncol,nlay);
  for (int igas = 1 ; igas <= ngas ; igas++) {
    real2d vmrtmp("vmrtmp",ncol,nlay);
    parallel_for( Bounds<2>(nlay,ncol) , YAKL_LAMBDA (int ilay, int icol) {
      vmrtmp(icol,ilay) = gas_vmr(igas,icol,ilay);
    });
    gas_concs.set_vmr(gas_names(igas),vmrtmp);
  }

  // Optical properties arrays
  OpticalProps1scl optical_props;
  optical_props.alloc_1scl(ncol, nlay, k_dist_lw);

  OpticalProps1scl cld_props;
  cld_props.alloc_1scl(ncol, nlay, k_dist_lw);
  cld_props.tau = cld_tau;

  OpticalProps1scl aer_props;
  aer_props.alloc_1scl(ncol, nlay, k_dist_lw.get_band_lims_wavenumber());
  aer_props.tau = aer_tau;

  // Source function
  SourceFuncLW sources;
  sources.init(k_dist_lw);
  sources.alloc(ncol, nlay);

  // Gas optical depth -- pressure need to be expressed as Pa
  // The "real2d()" argument will be interpreted as a missing optional parameter
  k_dist_lw.gas_optics(top_at_1, p_lay, p_lev, t_lay, t_sfc, gas_concs, optical_props, sources, real2d(), t_lev );

  // Weights and angle secants for first order (k=1) Gaussian quadrature.
  //   Values from Table 2, Clough et al, 1992, doi:10.1029/92JD01419
  //   after Abramowitz & Stegun 1972, page 921
  int constexpr max_gauss_pts = 4;
  realHost2d gauss_Ds_host ("gauss_Ds" ,max_gauss_pts,max_gauss_pts);
  gauss_Ds_host(1,1) = 1.66_wp      ; gauss_Ds_host(2,1) =         0._wp; gauss_Ds_host(3,1) =         0._wp; gauss_Ds_host(4,1) =         0._wp;
  gauss_Ds_host(1,2) = 1.18350343_wp; gauss_Ds_host(2,2) = 2.81649655_wp; gauss_Ds_host(3,2) =         0._wp; gauss_Ds_host(4,2) =         0._wp;
  gauss_Ds_host(1,3) = 1.09719858_wp; gauss_Ds_host(2,3) = 1.69338507_wp; gauss_Ds_host(3,3) = 4.70941630_wp; gauss_Ds_host(4,3) =         0._wp;
  gauss_Ds_host(1,4) = 1.06056257_wp; gauss_Ds_host(2,4) = 1.38282560_wp; gauss_Ds_host(3,4) = 2.40148179_wp; gauss_Ds_host(4,4) = 7.15513024_wp;

  realHost2d gauss_wts_host("gauss_wts",max_gauss_pts,max_gauss_pts);
  gauss_wts_host(1,1) = 0.5_wp         ; gauss_wts_host(2,1) = 0._wp          ; gauss_wts_host(3,1) = 0._wp          ; gauss_wts_host(4,1) = 0._wp          ;
  gauss_wts_host(1,2) = 0.3180413817_wp; gauss_wts_host(2,2) = 0.1819586183_wp; gauss_wts_host(3,2) = 0._wp          ; gauss_wts_host(4,2) = 0._wp          ;
  gauss_wts_host(1,3) = 0.2009319137_wp; gauss_wts_host(2,3) = 0.2292411064_wp; gauss_wts_host(3,3) = 0.0698269799_wp; gauss_wts_host(4,3) = 0._wp          ;
  gauss_wts_host(1,4) = 0.1355069134_wp; gauss_wts_host(2,4) = 0.2034645680_wp; gauss_wts_host(3,4) = 0.1298475476_wp; gauss_wts_host(4,4) = 0.0311809710_wp;

  real2d gauss_Ds ("gauss_Ds" ,max_gauss_pts,max_gauss_pts);
  real2d gauss_wts("gauss_wts",max_gauss_pts,max_gauss_pts);
  gauss_Ds_host .deep_copy_to(gauss_Ds );
  gauss_wts_host.deep_copy_to(gauss_wts);

  // Clear sky fluxes (gases + aerosols)
  aer_props.increment(optical_props);
  rte_lw( max_gauss_pts , gauss_Ds , gauss_wts , optical_props, top_at_1, sources, sfc_emis, clrsky_fluxes, real2d(), n_gauss_angles );

  // All-sky fluxes (clear skies + clouds)
  cld_props.increment(optical_props);
  rte_lw( max_gauss_pts , gauss_Ds , gauss_wts , optical_props, top_at_1, sources, sfc_emis, allsky_fluxes, real2d(), n_gauss_angles );

  // Clean up
  sources.finalize();
}


// TODO: create string1d "gas_names" before calling this function
extern "C" void rrtmgp_run_sw_cpp(int ngas, int ncol, int nlay, int nbnd, int ngpt, real *gas_vmr_p, real *p_lay_p, real *t_lay_p, real *p_lev_p,
                                  real *mu0_p, real *sfc_alb_dir_p, real *sfc_alb_dif_p, real *cld_tau_p, real *cld_ssa_p, real *cld_asm_p,
                                  real *aer_tau_p, real *aer_ssa_p, real *aer_asm_p, real *allsky_flux_up_p, real *allsky_flux_dn_p,
                                  real *allsky_flux_net_p, real *allsky_bnd_flux_up_p, real *allsky_bnd_flux_dn_p, real *allsky_bnd_flux_net_p,
                                  real *allsky_bnd_flux_dn_dir_p, real *clrsky_flux_up_p, real *clrsky_flux_dn_p, real *clrsky_flux_net_p,
                                  real *clrsky_bnd_flux_up_p, real *clrsky_bnd_flux_dn_p, real *clrsky_bnd_flux_net_p, real *clrsky_bnd_flux_dn_dir_p,
                                  real tsi_scaling ) {
  // Wrap pointers in YAKL Arrays
  real3d gas_vmr                = realHost3d("gas_vmr               ",gas_vmr_p               ,ngas,ncol,nlay  ).createDeviceCopy();  
  real2d p_lay                  = realHost2d("p_lay                 ",p_lay_p                 ,ncol,nlay       ).createDeviceCopy();  
  real2d t_lay                  = realHost2d("t_lay                 ",t_lay_p                 ,ncol,nlay       ).createDeviceCopy();  
  real2d p_lev                  = realHost2d("p_lev                 ",p_lev_p                 ,ncol,nlay+1     ).createDeviceCopy();  
  real1d mu0                    = realHost1d("mu0                   ",mu0_p                   ,ncol            ).createDeviceCopy();  
  real2d sfc_alb_dir            = realHost2d("sfc_alb_dir           ",sfc_alb_dir_p           ,nbnd,ncol       ).createDeviceCopy();  
  real2d  sfc_alb_dif           = realHost2d(" sfc_alb_dif          ", sfc_alb_dif_p          ,nbnd,ncol       ).createDeviceCopy();  
  real3d cld_tau                = realHost3d("cld_tau               ",cld_tau_p               ,ncol,nlay,ngpt  ).createDeviceCopy();  
  real3d cld_ssa                = realHost3d("cld_ssa               ",cld_ssa_p               ,ncol,nlay,ngpt  ).createDeviceCopy();  
  real3d cld_asm                = realHost3d("cld_asm               ",cld_asm_p               ,ncol,nlay,ngpt  ).createDeviceCopy();  
  real3d aer_tau                = realHost3d("aer_tau               ",aer_tau_p               ,ncol,nlay,nbnd  ).createDeviceCopy();  
  real3d aer_ssa                = realHost3d("aer_ssa               ",aer_ssa_p               ,ncol,nlay,nbnd  ).createDeviceCopy();  
  real3d aer_asm                = realHost3d("aer_asm               ",aer_asm_p               ,ncol,nlay,nbnd  ).createDeviceCopy();  
  real2d allsky_flux_up         = realHost2d("allsky_flux_up        ",allsky_flux_up_p        ,ncol,nlay+1     ).createDeviceCopy();  
  real2d allsky_flux_dn         = realHost2d("allsky_flux_dn        ",allsky_flux_dn_p        ,ncol,nlay+1     ).createDeviceCopy();  
  real2d allsky_flux_net        = realHost2d("allsky_flux_net       ",allsky_flux_net_p       ,ncol,nlay+1     ).createDeviceCopy();  
  real2d clrsky_flux_up         = realHost2d("clrsky_flux_up        ",clrsky_flux_up_p        ,ncol,nlay+1     ).createDeviceCopy();  
  real2d clrsky_flux_dn         = realHost2d("clrsky_flux_dn        ",clrsky_flux_dn_p        ,ncol,nlay+1     ).createDeviceCopy();  
  real2d clrsky_flux_net        = realHost2d("clrsky_flux_net       ",clrsky_flux_net_p       ,ncol,nlay+1     ).createDeviceCopy();  
  real3d allsky_bnd_flux_up     = realHost3d("allsky_bnd_flux_up    ",allsky_bnd_flux_up_p    ,ncol,nlay+1,nbnd).createDeviceCopy();  
  real3d allsky_bnd_flux_dn     = realHost3d("allsky_bnd_flux_dn    ",allsky_bnd_flux_dn_p    ,ncol,nlay+1,nbnd).createDeviceCopy();  
  real3d allsky_bnd_flux_net    = realHost3d("allsky_bnd_flux_net   ",allsky_bnd_flux_net_p   ,ncol,nlay+1,nbnd).createDeviceCopy();  
  real3d allsky_bnd_flux_dn_dir = realHost3d("allsky_bnd_flux_dn_dir",allsky_bnd_flux_dn_dir_p,ncol,nlay+1,nbnd).createDeviceCopy();  
  real3d clrsky_bnd_flux_up     = realHost3d("clrsky_bnd_flux_up    ",clrsky_bnd_flux_up_p    ,ncol,nlay+1,nbnd).createDeviceCopy();  
  real3d clrsky_bnd_flux_dn     = realHost3d("clrsky_bnd_flux_dn    ",clrsky_bnd_flux_dn_p    ,ncol,nlay+1,nbnd).createDeviceCopy();  
  real3d clrsky_bnd_flux_net    = realHost3d("clrsky_bnd_flux_net   ",clrsky_bnd_flux_net_p   ,ncol,nlay+1,nbnd).createDeviceCopy();  
  real3d clrsky_bnd_flux_dn_dir = realHost3d("clrsky_bnd_flux_dn_dir",clrsky_bnd_flux_dn_dir_p,ncol,nlay+1,nbnd).createDeviceCopy();  

  // Flag for TOA->SFC or SFC->TOA
  realHost2d p_lay_host = p_lay.createHostCopy();
  bool top_at_1 = p_lay_host(1,1) < p_lay_host(1,nlay);

  // Assign Arrays to the fluxes objects
  FluxesByband allsky_fluxes;
  allsky_fluxes.flux_up         = allsky_flux_up        ;
  allsky_fluxes.flux_dn         = allsky_flux_dn        ;
  allsky_fluxes.flux_net        = allsky_flux_net       ;
  allsky_fluxes.bnd_flux_up     = allsky_bnd_flux_up    ;
  allsky_fluxes.bnd_flux_dn     = allsky_bnd_flux_dn    ;
  allsky_fluxes.bnd_flux_net    = allsky_bnd_flux_net   ;
  allsky_fluxes.bnd_flux_dn_dir = allsky_bnd_flux_dn_dir;

  FluxesByband clrsky_fluxes;
  clrsky_fluxes.flux_up         = clrsky_flux_up        ;
  clrsky_fluxes.flux_dn         = clrsky_flux_dn        ;
  clrsky_fluxes.flux_net        = clrsky_flux_net       ;
  clrsky_fluxes.bnd_flux_up     = clrsky_bnd_flux_up    ;
  clrsky_fluxes.bnd_flux_dn     = clrsky_bnd_flux_dn    ;
  clrsky_fluxes.bnd_flux_net    = clrsky_bnd_flux_net   ;
  clrsky_fluxes.bnd_flux_dn_dir = clrsky_bnd_flux_dn_dir;

  // Setup gas concentrations
  GasConcs gas_concs;
  gas_concs.init(gas_names,ncol,nlay);
  for (int igas=1 ; igas <= ngas ; igas++) {
    real2d vmrtmp("vmrtmp",ncol,nlay);
    parallel_for( Bounds<2>(nlay,ncol) , YAKL_LAMBDA (int ilay, int icol) {
      vmrtmp(icol,ilay) = gas_vmr(igas,icol,ilay);
    });
    gas_concs.set_vmr(gas_names(igas), vmrtmp);
  }

  // Optical properties arrays
  OpticalProps2str optical_props;
  optical_props.alloc_2str(ncol, nlay, k_dist_sw);

  OpticalProps2str cloud_props;
  cloud_props.alloc_2str(ncol, nlay, k_dist_sw);
  
  OpticalProps2str aer_props;
  aer_props.alloc_2str(ncol, nlay, k_dist_sw.get_band_lims_wavenumber());

  // Populate optical properties
  cloud_props.tau = cld_tau;
  cloud_props.ssa = cld_ssa;
  cloud_props.g   = cld_asm;
  aer_props.tau = aer_tau;
  aer_props.ssa = aer_ssa;
  aer_props.g   = aer_asm;

  // Delta scale
  cloud_props.delta_scale();

  // TOA flux
  real2d toa_flux("toa_flux",ncol,ngpt);

  // Gas optical depth -- pressure need to be expressed as Pa
  k_dist_sw.gas_optics(top_at_1, p_lay, p_lev, t_lay, gas_concs, optical_props, toa_flux);

  parallel_for( Bounds<2>(ngpt,ncol) , YAKL_LAMBDA(int igpt, int icol) {
    toa_flux(icol,igpt) = toa_flux(icol,igpt) * tsi_scaling;
  });

  // Clear sky is gases + aerosols (if they're supplied)
  aer_props.increment(optical_props);
  rte_sw( optical_props, top_at_1, mu0, toa_flux, sfc_alb_dir, sfc_alb_dif, clrsky_fluxes);

  // All-sky fluxes = clear skies + clouds
  cloud_props.increment(optical_props);
  rte_sw( optical_props, top_at_1, mu0, toa_flux, sfc_alb_dir, sfc_alb_dif, allsky_fluxes);
}

