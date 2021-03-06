/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkResampleToImage.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkResampleToImage.h"

#include "vtkCharArray.h"
#include "vtkCompositeDataProbeFilter.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkIdList.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnsignedCharArray.h"

#include <algorithm>


vtkObjectFactoryNewMacro(vtkResampleToImage);

//----------------------------------------------------------------------------
vtkResampleToImage::vtkResampleToImage()
  : UseInputBounds(true)
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);

  this->SamplingBounds[0] = this->SamplingBounds[2] = this->SamplingBounds[4] = 0;
  this->SamplingBounds[1] = this->SamplingBounds[3] = this->SamplingBounds[5] = 1;
  this->SamplingDimensions[0] = this->SamplingDimensions[1] =
  this->SamplingDimensions[2] = 10;
}

//----------------------------------------------------------------------------
vtkResampleToImage::~vtkResampleToImage()
{
}

//----------------------------------------------------------------------------
vtkImageData* vtkResampleToImage::GetOutput()
{
  return vtkImageData::SafeDownCast(this->GetOutputDataObject(0));
}

//----------------------------------------------------------------------------
int vtkResampleToImage::ProcessRequest(vtkInformation* request,
                                        vtkInformationVector** inputVector,
                                        vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
    {
    return this->RequestData(request, inputVector, outputVector);
    }

  // execute information
  if(request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
    {
    return this->RequestInformation(request, inputVector, outputVector);
    }

  // propagate update extent
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkResampleToImage::RequestInformation(vtkInformation *,
                                            vtkInformationVector **,
                                            vtkInformationVector *outputVector)
{
  int wholeExtent[6] = { 0, this->SamplingDimensions[0] - 1,
                         0, this->SamplingDimensions[1] - 1,
                         0, this->SamplingDimensions[2] - 1 };

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent, 6);

  return 1;
}

//----------------------------------------------------------------------------
int vtkResampleToImage::RequestUpdateExtent(vtkInformation *,
                                             vtkInformationVector **inputVector,
                                             vtkInformationVector *)
{
  // This filter always asks for whole extent downstream. To resample
  // a subset of a structured input, you need to use ExtractVOI.
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  inInfo->Remove(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  if (inInfo->Has(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()))
    {
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
                inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()),
                6);
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkResampleToImage::FillInputPortInformation(int vtkNotUsed(port),
                                                  vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCompositeDataSet");
  return 1;
}

//----------------------------------------------------------------------------
int vtkResampleToImage::FillOutputPortInformation(int vtkNotUsed(port),
                                                   vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkResampleToImage::SetBlankPointsAndCells(vtkImageData *data,
                                                const char *maskArrayName)
{
  data->AllocatePointGhostArray();
  vtkUnsignedCharArray *pointGhostArray = data->GetPointGhostArray();

  data->AllocateCellGhostArray();
  vtkUnsignedCharArray *cellGhostArray = data->GetCellGhostArray();

  vtkPointData *pd = data->GetPointData();
  vtkCharArray *maskArray = vtkCharArray::SafeDownCast(pd->GetArray(maskArrayName));
  char *mask = maskArray->GetPointer(0);

  vtkNew<vtkIdList> pointCells;
  pointCells->Allocate(8);

  vtkIdType numPoints = data->GetNumberOfPoints();
  for (vtkIdType i = 0; i < numPoints; ++i)
    {
    if (!mask[i])
      {
      pointGhostArray->SetValue(i, pointGhostArray->GetValue(i) |
                                   vtkDataSetAttributes::HIDDENPOINT);

      data->GetPointCells(i, pointCells.GetPointer());
      vtkIdType numCells = pointCells->GetNumberOfIds();
      for (vtkIdType j = 0; j < numCells; ++j)
        {
        vtkIdType cellId = pointCells->GetId(j);
        cellGhostArray->SetValue(cellId, cellGhostArray->GetValue(cellId) |
                                         vtkDataSetAttributes::HIDDENPOINT);
        }
      }
    }
}

void vtkResampleToImage::GetCompositeDataSetBounds(vtkCompositeDataSet* data,
                                                   double bounds[6])
{
  bounds[0] = bounds[2] = bounds[4] = VTK_DOUBLE_MAX;
  bounds[1] = bounds[3] = bounds[5] = -VTK_DOUBLE_MAX;

  vtkCompositeDataIterator *iter = data->NewIterator();
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
    vtkDataSet *ds = vtkDataSet::SafeDownCast(iter->GetCurrentDataObject());
    if (!ds)
      {
      vtkErrorMacro("All leaves in the multiblock dataset must be vtkDataSet.");
      continue;
      }
    double b[6];
    ds->GetBounds(b);
    for (int i = 0; i < 3; ++i)
      {
      bounds[2*i] = vtkMath::Min(bounds[2*i], b[2*i]);
      bounds[2*i + 1] = vtkMath::Max(bounds[2*i + 1], b[2*i + 1]);
      }
    }
  iter->Delete();
}

//----------------------------------------------------------------------------
int vtkResampleToImage::RequestData(vtkInformation *vtkNotUsed(request),
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkImageData *output = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  if (this->SamplingDimensions[0] <= 0 ||
      this->SamplingDimensions[1] <= 0 ||
      this->SamplingDimensions[2] <= 0)
    {
    return 1;
    }

  double inputBounds[6];
  if (vtkDataSet::SafeDownCast(input))
    {
    vtkDataSet::SafeDownCast(input)->GetBounds(inputBounds);
    }
  else
    {
    this->GetCompositeDataSetBounds(vtkCompositeDataSet::SafeDownCast(input),
                                    inputBounds);
    }

  // compute bounds and extent where probing should be performed
  double *wholeBounds = this->UseInputBounds ? inputBounds : this->SamplingBounds;
  double origin[3] = { wholeBounds[0], wholeBounds[2], wholeBounds[4] };
  double spacing[3];
  for (int i = 0; i < 3; ++i)
    {
    spacing[i] = (this->SamplingDimensions[i] == 1) ? 0 :
                 ((wholeBounds[i*2 + 1] - wholeBounds[i*2]) /
                  static_cast<double>(this->SamplingDimensions[i] - 1));
    }

  int extent[6];
  if (outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
    {
    int *updateExtent = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    std::copy(updateExtent, updateExtent + 6, extent);
    }
  else
    {
    extent[0] = extent[2] = extent[4] = 0;
    extent[1] = this->SamplingDimensions[0] - 1;
    extent[3] = this->SamplingDimensions[1] - 1;
    extent[5] = this->SamplingDimensions[2] - 1;
    }

  // perform probing
  vtkNew<vtkImageData> structure;
  structure->SetOrigin(origin);
  structure->SetSpacing(spacing);
  structure->SetExtent(extent);

  vtkNew<vtkCompositeDataProbeFilter> prober;
  prober->SetInputData(structure.GetPointer());
  prober->SetSourceData(input);
  prober->Update();

  const char *maskArrayName = prober->GetValidPointMaskArrayName();
  output->ShallowCopy(prober->GetOutput());
  SetBlankPointsAndCells(output, maskArrayName);
  return 1;
}

//----------------------------------------------------------------------------
void vtkResampleToImage::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "UseInputBounds " << this->UseInputBounds << endl;
  os << indent << "SamplingBounds ["
     << this->SamplingBounds[0] << ", "
     << this->SamplingBounds[1] << ", "
     << this->SamplingBounds[2] << ", "
     << this->SamplingBounds[3] << ", "
     << this->SamplingBounds[4] << ", "
     << this->SamplingBounds[5] << "]"
     << endl;
  os << indent << "SamplingDimensions "
     << this->SamplingDimensions[0] << " x "
     << this->SamplingDimensions[1] << " x "
     << this->SamplingDimensions[2] << endl;
}
