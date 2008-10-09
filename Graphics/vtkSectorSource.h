/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSectorSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSectorSource - create a sector of a disk
// .SECTION Description
// vtkSectorSource creates a sector of a polygonal disk. The 
// disk has zero height. The user can specify the inner and outer radius
// of the disk, the z-coordinate, and the radial and 
// circumferential resolution of the polygonal representation. 
// .SECTION See Also
// vtkLinearExtrusionFilter

#ifndef __vtkSectorSource_h
#define __vtkSectorSource_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkSectorSource : public vtkPolyDataAlgorithm 
{
public:
  static vtkSectorSource *New();
  vtkTypeRevisionMacro(vtkSectorSource,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify inner radius of the sector.
  vtkSetClampMacro(InnerRadius,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(InnerRadius,double);

  // Description:
  // Specify outer radius of the sector.
  vtkSetClampMacro(OuterRadius,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(OuterRadius,double);

  // Description:
  // Specify the z coordinate of the sector.
  vtkSetClampMacro(ZCoord,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(ZCoord,double);

  // Description:
  // Set the number of points in radius direction.
  vtkSetClampMacro(RadialResolution,int,1,VTK_LARGE_INTEGER)
  vtkGetMacro(RadialResolution,int);

  // Description:
  // Set the number of points in circumferential direction.
  vtkSetClampMacro(CircumferentialResolution,int,3,VTK_LARGE_INTEGER)
  vtkGetMacro(CircumferentialResolution,int);

  // Description:
  // Set the start angle of the sector.
  vtkSetClampMacro(StartAngle,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(StartAngle,double);

  // Description:
  // Set the end angle of the sector.
  vtkSetClampMacro(EndAngle,double,0.0,VTK_DOUBLE_MAX)
  vtkGetMacro(EndAngle,double);

protected:
  vtkSectorSource();
  ~vtkSectorSource() {};

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  double InnerRadius;
  double OuterRadius;
  double ZCoord;
  int RadialResolution;
  int CircumferentialResolution;
  double StartAngle;
  double EndAngle;
  
private:
  vtkSectorSource(const vtkSectorSource&);  // Not implemented.
  void operator=(const vtkSectorSource&);  // Not implemented.
};

#endif
