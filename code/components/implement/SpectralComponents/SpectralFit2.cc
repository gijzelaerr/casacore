//# SpectralFit2.cc: Least Squares fitting of spectral elements: templated part
//# Copyright (C) 2001,2002
//# Associated Universities, Inc. Washington DC, USA.
//#
//# This library is free software; you can redistribute it and/or modify it
//# under the terms of the GNU Library General Public License as published by
//# the Free Software Foundation; either version 2 of the License, or (at your
//# option) any later version.
//#
//# This library is distributed in the hope that it will be useful, but WITHOUT
//# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
//# License for more details.
//#
//# You should have received a copy of the GNU Library General Public License
//# along with this library; if not, write to the Free Software Foundation,
//# Inc., 675 Massachusetts Ave, Cambridge, MA 02139, USA.
//#
//# Correspondence concerning AIPS++ should be addressed as follows:
//#        Internet email: aips2-request@nrao.edu.
//#        Postal address: AIPS++ Project Office
//#                        National Radio Astronomy Observatory
//#                        520 Edgemont Road
//#                        Charlottesville, VA 22903-2475 USA
//#
//# $Id$

//# Includes
#include <trial/SpectralComponents/SpectralFit.h>
#include <aips/Arrays/Vector.h>
#include <aips/Functionals/CompoundFunction.h>
#include <aips/Functionals/CompoundParam.h>
#include <aips/Functionals/Gaussian1D.h>
#include <aips/Functionals/Polynomial.h>
#include <trial/Fitting/NonLinearFitLM.h>
#include <trial/SpectralComponents/SpectralElement.h>

//# Templated member functions
template <class MT>
Bool SpectralFit::fit(const Vector<MT> &y,
		      const Vector<MT> &x,
		      const Vector<Bool> *const mask) {
  // The fitter
  NonLinearFitLM<MT> fitter;
  iter_p = 0;
  // The functions to fit
  Gaussian1D<AutoDiff<MT> > gauss;
  Polynomial<AutoDiff<MT> > poly;
  CompoundFunction<AutoDiff<MT> > func;
  // Initial guess
  uInt npar(0);
  for (uInt i=0; i<slist_p.nelements(); i++) {
    if (slist_p[i].getType() == SpectralElement::GAUSSIAN) {
      gauss[0] = AutoDiff<MT>(slist_p[i].getAmpl(), gauss.nparameters(), 0);
      gauss[1] = AutoDiff<MT>(slist_p[i].getCenter(), gauss.nparameters(), 1);
      gauss[2] = AutoDiff<MT>(slist_p[i].getFWHM(), gauss.nparameters(), 2);
      gauss.mask(0) = !slist_p[i].fixedAmpl();
      gauss.mask(1) = !slist_p[i].fixedCenter();
      gauss.mask(2) = !slist_p[i].fixedFWHM();
      func.addFunction(gauss);
      npar += gauss.nparameters();
    } else if (slist_p[i].getType() == SpectralElement::POLYNOMIAL) {
      npar += slist_p[i].getDegree()+1;
      Polynomial<AutoDiff<MT> > poly(slist_p[i].getDegree());
      for (uInt j=0; j<poly.nparameters(); ++j) {
	poly[j] = AutoDiff<MT>(0, poly.nparameters(), j);
	poly.mask(j) = !slist_p[i].fixed()(j);
      };
      func.addFunction(poly);
    };
  };
  fitter.setFunction(func);
  // Max. number of iterations
  fitter.setMaxIter(50+ slist_p.nelements()*10);
  // Convergence criterium
  fitter.setCriteria(0.001);
  // Fit
  Vector<MT> sol;
  Vector<MT> err;
  Vector<MT> sigma(x.nelements());
  sigma = 1.0;
  sol = fitter.fit(x, y, sigma, mask);
  err = fitter.errors();
  // Number of iterations
  iter_p = fitter.currentIteration();
  uInt j = 0;
  Vector<Double> tmp, terr;
  for (uInt i=0; i<slist_p.nelements(); i++) {
    tmp.resize(slist_p[i].getOrder());
    terr.resize(slist_p[i].getOrder());
    for (uInt k=0; k<slist_p[i].getOrder(); k++) {
      if (k==2 && slist_p[i].getType() == SpectralElement::GAUSSIAN) {
	tmp(k) = sol(j) / SpectralElement::SigmaToFWHM;
	terr(k) = err(j++)  / SpectralElement::SigmaToFWHM;
      } else { /// Get rid of / above after changing element contents
	tmp(k) = sol(j);
	terr(k) = err(j++);
      };
    };
    slist_p[i].set(tmp);
    slist_p[i].setError(terr);
  };
  return fitter.converged();
}

//# Cater for Double and Float profiles
template Bool SpectralFit::fit(Vector<Double> const &,
			       Vector<Double> const &,
			       Vector<Bool> const *);
template Bool SpectralFit::fit(Vector<Float> const &,
			       Vector<Float> const &,
			       Vector<Bool> const *);
