/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */

// $Revision: 6223 $
// $Date: 2016-03-01 16:32:50 -0800 (Tue, 01 Mar 2016) $
// $URL: svn://peera.berkeley.edu/usr/local/svn/OpenSees/trunk/SRC/analysis/integrator/HHTExplicit_TP.h $

#ifndef HHTExplicit_TP_h
#define HHTExplicit_TP_h

// Written: Andreas Schellenberg (andreas.schellenberg@gmail.com)
// Created: 02/05
// Revision: A
//
// Description: This file contains the class definition for HHTExplicit_TP.
// HHTExplicit_TP is an algorithmic class for performing a transient analysis
// using the HHTExplicit_TP integration scheme (beta = 0) based on the
// trapezoidal rule.

#include <TransientIntegrator.h>

class DOF_Group;
class FE_Element;
class Vector;

class HHTExplicit_TP : public TransientIntegrator
{
public:
    // constructors
    HHTExplicit_TP();
    HHTExplicit_TP(double alpha);
    HHTExplicit_TP(double alpha, double gamma);
    
    // destructor
    ~HHTExplicit_TP();
    
    // method to set up the system of equations
    int formUnbalance(void);
    
    // methods which define what the FE_Element and DOF_Groups add
    // to the system of equation object.
    int formEleTangent(FE_Element *theEle);
    int formNodTangent(DOF_Group *theDof);
    int formEleResidual(FE_Element *theEle);
    int formNodUnbalance(DOF_Group *theDof);
    
    int domainChanged(void);
    int newStep(double deltaT);
    int revertToLastStep(void);
    int update(const Vector &aiPlusOne);
    int commit(void);
    
    virtual int sendSelf(int commitTag, Channel &theChannel);
    virtual int recvSelf(int commitTag, Channel &theChannel, FEM_ObjectBroker &theBroker);
    
    void Print(OPS_Stream &s, int flag = 0);
    
protected:
    
private:
    double alpha;
    double gamma;
    double deltaT;
    
    int updateCount;                // method should only have one update per step
    double c2, c3;                  // some constants we need to keep
    double alphaD, alphaR, alphaP;  // weighting factors we need to keep
    Vector *Ut, *Utdot, *Utdotdot;  // response quantities at time t
    Vector *U, *Udot, *Udotdot;     // response quantities at time t + deltaT
    Vector *Put;                    // unbalance at time t
};

#endif
