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

// $Revision: 6501 $
// $Date: 2016-12-14 18:09:33 -0800 (Wed, 14 Dec 2016) $
// $URL: svn://peera.berkeley.edu/usr/local/svn/OpenSees/trunk/SRC/element/generic/GenericCopy.cpp $

// Written: Andreas Schellenberg (andreas.schellenberg@gmail.com)
// Created: 11/06
// Revision: A
//
// Description: This file contains the implementation of the GenericCopy class.

#include "GenericCopy.h"

#include <Domain.h>
#include <Node.h>
#include <Channel.h>
#include <FEM_ObjectBroker.h>
#include <Renderer.h>
#include <Information.h>
#include <ElementResponse.h>

#include <math.h>
#include <stdlib.h>
#include <string.h>


// responsible for allocating the necessary space needed
// by each object and storing the tags of the end nodes.
GenericCopy::GenericCopy(int tag, ID nodes, int srctag)
    : Element(tag, ELE_TAG_GenericCopy),
    connectedExternalNodes(nodes), numExternalNodes(0), numDOF(0),
    srcTag(srctag), theSource(0), theMatrix(1,1), theVector(1),
    theLoad(1), theInitStiff(1,1), theMass(1,1),
    initStiffFlag(false), massFlag(false)
{
    // initialize nodes
    numExternalNodes = connectedExternalNodes.Size();
    theNodes = new Node* [numExternalNodes];
    if (!theNodes)  {
        opserr << "GenericCopy::GenericCopy() "
            << "- failed to create node array\n";
        exit(-1);
    }
    
    // set node pointers to NULL
    int i;
    for (i=0; i<numExternalNodes; i++)
        theNodes[i] = 0;
}


// invoked by a FEM_ObjectBroker - blank object that recvSelf
// needs to be invoked upon
GenericCopy::GenericCopy()
    : Element(0, ELE_TAG_GenericCopy),
    connectedExternalNodes(1), numExternalNodes(0), numDOF(0),
    srcTag(0), theSource(0), theMatrix(1,1), theVector(1),
    theLoad(1), theInitStiff(1,1), theMass(1,1),
    initStiffFlag(false), massFlag(false)
{
    // initialize variables
    theNodes = 0;
}


// delete must be invoked on any objects created by the object.
GenericCopy::~GenericCopy()
{
    // invoke the destructor on any objects created by the object
    // that the object still holds a pointer to
    if (theNodes != 0)
        delete [] theNodes;
}


int GenericCopy::getNumExternalNodes() const
{
    return numExternalNodes;
}


const ID& GenericCopy::getExternalNodes()
{
    return connectedExternalNodes;
}


Node** GenericCopy::getNodePtrs()
{
    return theNodes;
}


int GenericCopy::getNumDOF()
{
    return numDOF;
}


// to set a link to the enclosing Domain and to set the node pointers.
void GenericCopy::setDomain(Domain *theDomain)
{
    // check Domain is not null - invoked when object removed from a domain
    int i;
    if (!theDomain)  {
        for (i=0; i<numExternalNodes; i++)
            theNodes[i] = 0;
        return;
    }
    
    // get a pointer to the source element
    theSource = theDomain->getElement(srcTag);
    if (theSource == 0) {
        opserr << "GenericCopy::setDomain() "
            << "- failed to get a pointer to the source "
            << "element with tag " << srcTag << endln;
        return;
    }
    
    // check we got correct number of nodes
    if (numExternalNodes != theSource->getNumExternalNodes()) {
        opserr << "GenericCopy::setDomain() "
            << "- number of external nodes of copy do not "
            << "agree with source\n";
        return;
    }
    
    // now set the node pointers
    for (i=0; i<numExternalNodes; i++)
        theNodes[i] = theDomain->getNode(connectedExternalNodes(i));
    
    // if can't find all - send a warning message
    for (i=0; i<numExternalNodes; i++)  {
        if (!theNodes[i])  {
            opserr << "GenericCopy::setDomain() - Nd" << i << ": " 
                << connectedExternalNodes(i) << " does not exist in the "
                << "model for GenericCopy ele: " << this->getTag() << endln;
            return;
        }
    }
    
    // now determine the number of dof
    numDOF = 0;
    for (i=0; i<numExternalNodes; i++)  {
        numDOF += theNodes[i]->getNumberDOF();
    }
    if (numDOF != theSource->getNumDOF()) {
        opserr << "GenericCopy::setDomain() "
            << "- number of DOFs of copy do not "
            << "agree with source\n";
        return;
    }
    
    // set the matrix and vector sizes and zero them
    theMatrix.resize(numDOF,numDOF);
    theMatrix.Zero();
    theVector.resize(numDOF);
    theVector.Zero();
    theLoad.resize(numDOF);
    theLoad.Zero();
    theInitStiff.resize(numDOF,numDOF);
    theInitStiff.Zero();
    theMass.resize(numDOF,numDOF);
    theMass.Zero();
    
    // call the base class method
    this->DomainComponent::setDomain(theDomain);
}


int GenericCopy::commitState()
{
    // does nothing
    return 0;
}


int GenericCopy::revertToLastCommit()
{
    // does nothing
    return 0;
}


int GenericCopy::revertToStart()
{
    // does nothing
    return 0;
}


int GenericCopy::update()
{
    // does nothing
    return 0;
}


const Matrix& GenericCopy::getTangentStiff()
{
    // zero the matrix
    theMatrix.Zero();
    
    // get tangent stiffness matrix from source element
    theMatrix = theSource->getTangentStiff();
    
    return theMatrix;
}


const Matrix& GenericCopy::getInitialStiff()
{
    if (initStiffFlag == false)  {
        // zero the matrix
        theInitStiff.Zero();
        
        // get initial stiffness matrix from source element
        theInitStiff = theSource->getInitialStiff();
        initStiffFlag = true;
    }
    
    return theInitStiff;
}


const Matrix& GenericCopy::getDamp()
{
    // zero the matrix
    theMatrix.Zero();
    
    // get damping matrix from source element
    theMatrix = theSource->getDamp();
    
    return theMatrix;
}


const Matrix& GenericCopy::getMass()
{
    if (massFlag == false)  {
        // zero the matrix
        theMass.Zero();
        
        // get mass matrix from source element
        theMass = theSource->getMass();
        massFlag = true;
    }
    
    return theMass;
}


void GenericCopy::zeroLoad()
{
    theLoad.Zero();
}


int GenericCopy::addLoad(ElementalLoad *theLoad, double loadFactor)
{
    opserr <<"GenericCopy::addLoad() - "
        << "load type unknown for element: "
        << this->getTag() << endln;
    
    return -1;
}


int GenericCopy::addInertiaLoadToUnbalance(const Vector &accel)
{
    if (massFlag == false)
        this->getMass();
    
    int ndim = 0, i;
    Vector Raccel(numDOF);
    
    // assemble Raccel vector
    for (i=0; i<numExternalNodes; i++ )  {
        Raccel.Assemble(theNodes[i]->getRV(accel), ndim);
        ndim += theNodes[i]->getNumberDOF();
    }
    
    // want to add ( - fact * M R * accel ) to unbalance
    theLoad.addMatrixVector(1.0, theMass, Raccel, -1.0);
    
    return 0;
}


const Vector& GenericCopy::getResistingForce()
{
    // zero the residual
    theVector.Zero();
    
    // determine resisting forces in global system
    theVector = theSource->getResistingForce();
    
    return theVector;
}


const Vector& GenericCopy::getResistingForceIncInertia()
{
    theVector = this->getResistingForce();
    
    // subtract external load
    theVector.addVector(1.0, theLoad, -1.0);
    
    if (massFlag == false)
        this->getMass();
    
    int ndim = 0, i;
    Vector vel(numDOF), accel(numDOF);
    
    // add the damping forces from element damping
    Matrix C = this->getDamp();
    // assemble vel vector
    for (i=0; i<numExternalNodes; i++ )  {
        vel.Assemble(theNodes[i]->getTrialVel(), ndim);
        ndim += theNodes[i]->getNumberDOF();
    }
    theVector.addMatrixVector(1.0, C, vel, 1.0);
    
    // add inertia forces from element mass
    ndim = 0;
    // assemble accel vector
    for (i=0; i<numExternalNodes; i++ )  {
        accel.Assemble(theNodes[i]->getTrialAccel(), ndim);
        ndim += theNodes[i]->getNumberDOF();
    }
    theVector.addMatrixVector(1.0, theMass, accel, 1.0);
    
    return theVector;
}


int GenericCopy::sendSelf(int commitTag, Channel &sChannel)
{
    // send element parameters
    static ID idData(3);
    idData(0) = this->getTag();
    idData(1) = numExternalNodes;
    idData(2) = srcTag;
    sChannel.sendID(0, commitTag, idData);
    
    // send the end nodes
    sChannel.sendID(0, commitTag, connectedExternalNodes);
    
    return 0;
}


int GenericCopy::recvSelf(int commitTag, Channel &rChannel,
    FEM_ObjectBroker &theBroker)
{
    // delete dynamic memory
    if (theNodes != 0)
        delete [] theNodes;
    
    // receive element parameters
    static ID idData(3);
    rChannel.recvID(0, commitTag, idData);
    this->setTag(idData(0));
    numExternalNodes = idData(1);
    srcTag = idData(2);
    
    // initialize nodes and receive them
    connectedExternalNodes.resize(numExternalNodes);
    rChannel.recvID(0, commitTag, connectedExternalNodes);
    theNodes = new Node* [numExternalNodes];
    if (!theNodes)  {
        opserr << "GenericCopy::recvSelf() "
            << "- failed to create node array\n";
        return -1;
    }
    
    // set node pointers to NULL
    int i;
    for (i=0; i<numExternalNodes; i++)
        theNodes[i] = 0;
    
    return 0;
}


int GenericCopy::displaySelf(Renderer &theViewer,
    int displayMode, float fact, const char **modes, int numMode)
{
    int rValue = 0, i, j;

    if (numExternalNodes > 1)  {
        if (displayMode >= 0)  {
            for (i=0; i<numExternalNodes-1; i++)  {
                const Vector &end1Crd = theNodes[i]->getCrds();
                const Vector &end2Crd = theNodes[i+1]->getCrds();

                const Vector &end1Disp = theNodes[i]->getDisp();
                const Vector &end2Disp = theNodes[i+1]->getDisp();

                int end1NumCrds = end1Crd.Size();
                int end2NumCrds = end2Crd.Size();

                Vector v1(3), v2(3);

                for (j=0; j<end1NumCrds; j++)
                    v1(j) = end1Crd(j) + end1Disp(j)*fact;
                for (j=0; j<end2NumCrds; j++)
                    v2(j) = end2Crd(j) + end2Disp(j)*fact;

                rValue += theViewer.drawLine(v1, v2, 1.0, 1.0, this->getTag(), 0);
            }
        } else  {
            int mode = displayMode * -1;
            for (i=0; i<numExternalNodes-1; i++)  {
                const Vector &end1Crd = theNodes[i]->getCrds();
                const Vector &end2Crd = theNodes[i+1]->getCrds();

                const Matrix &eigen1 = theNodes[i]->getEigenvectors();
                const Matrix &eigen2 = theNodes[i+1]->getEigenvectors();

                int end1NumCrds = end1Crd.Size();
                int end2NumCrds = end2Crd.Size();

                Vector v1(3), v2(3);

                if (eigen1.noCols() >= mode)  {
                    for (j=0; j<end1NumCrds; j++)
                        v1(j) = end1Crd(j) + eigen1(j,mode-1)*fact;
                    for (j=0; j<end2NumCrds; j++)
                        v2(j) = end2Crd(j) + eigen2(j,mode-1)*fact;
                } else  {
                    for (j=0; j<end1NumCrds; j++)
                        v1(j) = end1Crd(j);
                    for (j=0; j<end2NumCrds; j++)
                        v2(j) = end2Crd(j);
                }

                rValue += theViewer.drawLine(v1, v2, 1.0, 1.0, this->getTag(), 0);
            }
        }
    }

    return rValue;
}


void GenericCopy::Print(OPS_Stream &s, int flag)
{
    int i;
    if (flag == 0)  {
        // print everything
        s << "Element: " << this->getTag() << endln;
        s << "  type: GenericCopy";
        for (i=0; i<numExternalNodes; i++ )
            s << ", Node" << i+1 << ": " << connectedExternalNodes(i);
        s << endln;
        s << "  source element: " << srcTag << endln;
        // determine resisting forces in global system
        s << "  resisting force: " << this->getResistingForce() << endln;
    } else if (flag == 1)  {
        // does nothing
    }
}


Response* GenericCopy::setResponse(const char **argv, int argc,
    OPS_Stream &output)
{
    Response *theResponse = 0;

    int i;
    char outputData[10];

    output.tag("ElementOutput");
    output.attr("eleType","GenericCopy");
    output.attr("eleTag",this->getTag());
    for (i=0; i<numExternalNodes; i++ )  {
        sprintf(outputData,"node%d",i+1);
        output.attr(outputData,connectedExternalNodes[i]);
    }

    // global forces
    if (strcmp(argv[0],"force") == 0 || strcmp(argv[0],"forces") == 0 ||
        strcmp(argv[0],"globalForce") == 0 || strcmp(argv[0],"globalForces") == 0)
    {
         for (i=0; i<numDOF; i++)  {
            sprintf(outputData,"P%d",i+1);
            output.tag("ResponseType",outputData);
        }
        theResponse = new ElementResponse(this, 1, theVector);
    }
    // local forces
    else if (strcmp(argv[0],"localForce") == 0 || strcmp(argv[0],"localForces") == 0)
    {
        for (i=0; i<numDOF; i++)  {
            sprintf(outputData,"p%d",i+1);
            output.tag("ResponseType",outputData);
        }
        theResponse = new ElementResponse(this, 2, theVector);
    }

    output.endTag(); // ElementOutput

    return theResponse;
}


int GenericCopy::getResponse(int responseID, Information &eleInfo)
{
    switch (responseID)  {
    case 1:  // global forces
        return eleInfo.setVector(this->getResistingForce());
        
    case 2:  // local forces
        return eleInfo.setVector(this->getResistingForce());
        
    default:
        return -1;
    }
}
