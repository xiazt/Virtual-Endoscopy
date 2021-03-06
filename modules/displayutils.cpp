#include "displayutils.h"
#include <iostream>
#include <vtkCamera.h>
#include <vtkJPEGReader.h>
#include <vtkTexture.h>
#include <vtkTextureMapToCylinder.h>
#include <vtkTextureMapToPlane.h>
#include <vtkTextureMapToSphere.h>
#include <vtkTransformTextureCoords.h>
vtkStandardNewMacro(CusInteractorStylePickPoint);

displayUtils::displayUtils(vtkRenderWindow *renWin)
    :m_renWindow(renWin),has_stl(false),has_line(false)
{
    vsp(m_renderer);
    vsp(m_stlactor);
    vsp(m_lineactor);

//    Instantiate(camera,vtkCamera);
//    camera->SetPosition(0,0,20);
//    camera->SetFocalPoint(0,0,0);
    vsp(m_light);
//    m_light->SetColor(1.,1.,1.);
//    m_light->SetIntensity(.5);
//    m_light->SetPosition(camera->GetPosition());
//    m_light->SetFocalPoint(camera->GetFocalPoint());

    m_renderer->AddLight(m_light);
//    m_renderer->SetActiveCamera(camera);
    m_renWindow->AddRenderer(m_renderer);
    m_iren = renWin->GetInteractor();

    vsp(m_lineInfoPointPicker);
    vsp(m_PointPickerInteractor);
    m_iren->SetInteractorStyle(m_PointPickerInteractor);
    m_PointPickerInteractor->PreparedRenderer(m_renderer);

    createSliderTool();

    m_renWindow->Render();

    m_centerline = new centerLineProc;
    m_currentRoamingStep = 2;
}

std::string displayUtils::GetRawFilename()
{
    return m_filename.second;
}

void displayUtils::OpenVolumeModel(std::string filename)
{
    Instantiate(reader, vtkMetaImageReader);
    reader -> SetFileName(filename.c_str());
    Instantiate(shiftscale, vtkImageShiftScale);
    // scale the volume data to unsigned char (0-255) before passing it to volume mapper
    shiftscale -> SetInputConnection(reader->GetOutputPort());
    shiftscale -> SetOutputScalarTypeToUnsignedChar();
    // Create transfer mapping scalar value to opacity.
    Instantiate(opacityTransferFunction, vtkPiecewiseFunction);
    opacityTransferFunction -> AddPoint(0.0, 0.0);
    opacityTransferFunction -> AddPoint(36.0, 0.125);
    opacityTransferFunction -> AddPoint(72.0, 0.25);
    opacityTransferFunction -> AddPoint(108.0, 0.375);
    opacityTransferFunction -> AddPoint(144.0, 0.5);
    opacityTransferFunction -> AddPoint(180.0, 0.625);
    opacityTransferFunction -> AddPoint(216.0, 0.75);
    opacityTransferFunction -> AddPoint(255.0, 0.875);

    // Create transfer mapping scalar value to color.
    Instantiate(colorTransferFunction, vtkColorTransferFunction);
    colorTransferFunction->AddRGBPoint(0.0, 0.0, 0.0, 0.0);
    colorTransferFunction->AddRGBPoint(36.0, 1.0, 0.0, 0.0);
    colorTransferFunction->AddRGBPoint(72.0, 1.0, 1.0, 0.0);
    colorTransferFunction->AddRGBPoint(108.0, 0.0, 1.0, 0.0);
    colorTransferFunction->AddRGBPoint(144.0, 0.0, 1.0, 1.0);
    colorTransferFunction->AddRGBPoint(180.0, 0.0, 0.0, 1.0);
    colorTransferFunction->AddRGBPoint(216.0, 1.0, 0.0, 1.0);
    colorTransferFunction->AddRGBPoint(255.0, 1.0, 1.0, 1.0);
    // set up volume property
    Instantiate(volumeProperty, vtkVolumeProperty);
    volumeProperty->SetColor(colorTransferFunction);
    volumeProperty->SetScalarOpacity(opacityTransferFunction);
    volumeProperty->ShadeOff();
    volumeProperty->SetInterpolationTypeToLinear();
    // set up mapper that renders the volume data.
    Instantiate(volumeMapper, vtkSmartVolumeMapper);
    volumeMapper->SetRequestedRenderMode(vtkSmartVolumeMapper::GPURenderMode);
    volumeMapper->SetInputConnection(shiftscale->GetOutputPort());

    m_mainvolume = vtkSmartPointer<vtkVolume>::New();
    m_mainvolume -> SetMapper(volumeMapper);
    m_mainvolume -> SetProperty(volumeProperty);

    // clear other current actor on renderer;
    SetMainModelOff();
    SetLineModelOff();
    m_renderer -> Clear();
    m_renderer -> AddVolume(m_mainvolume);



}

void displayUtils::OpenSegmentModel(std::string filename)
{
    //prepare input name(.stl and .mhd)
    m_filename.first = filename;
    int size = filename.size();
    m_filename.second = (m_filename.first).substr(0,size - 3) + "mhd";
    std::cout << "the mhd filename is " << m_filename.second << std::endl;
    Instantiate(reader,vtkSTLReader);
    reader -> SetFileName(filename.c_str());
    reader -> Update();
//  *****************add texture for model********************
//    Instantiate(tmapper,vtkTextureMapToSphere);
//    tmapper -> SetInputConnection(reader -> GetOutputPort());
// //    tmapper -> PreventSeamOn();
//    Instantiate(xform,vtkTransformTextureCoords);
//    xform -> SetInputConnection(tmapper->GetOutputPort());
//    xform -> SetScale(4,4,1);
//    Instantiate(jpgreader,vtkJPEGReader);
//    jpgreader -> SetFileName("D:\\3dresearch\\QtItkVtk\\test\\VirtualEndo\\res\\texture2.jpg");
//    jpgreader -> Update();
//    Instantiate(texture,vtkTexture);
//    texture -> SetInputConnection(jpgreader->GetOutputPort());
//    texture -> InterpolateOn();
//  ************************************************************


    //build connection
    Instantiate(mapper,vtkPolyDataMapper);
    mapper -> SetInputConnection(reader->GetOutputPort());
//    mapper -> SetInputConnection(xform -> GetOutputPort());
    m_stlactor -> SetMapper(mapper);
//    m_stlactor -> SetTexture(texture);
 //   m_stlactor->GetProperty()->SetColor(1., .0, .0);
    sliderWidget->EnabledOn();
    has_stl = true;
 //   m_renderer->ResetCamera();

}

void displayUtils::OpenCenterLineModel(std::string filename)
{
    //test centerline display


}

void displayUtils::centerLnDis(std::vector<OutputImageType::PointType> &inpoints)
{
    int num = inpoints.size();
    Instantiate(points,vtkPoints);
    Instantiate(vertices,vtkCellArray);
    Instantiate(lines,vtkCellArray);
    Instantiate(line,vtkLine);
    Instantiate(colors,vtkUnsignedCharArray);
    colors->SetNumberOfComponents(3);
    //green display each point.
    unsigned char green[3] = {0,255,0};
    std::vector<OutputImageType::PointType>::const_iterator it = inpoints.begin();

    for(unsigned int i = 0;i < num && it != inpoints.end();i++,it++){
        vtkIdType pid[1];
        pid[0] = points->InsertNextPoint((*it)[0],(*it)[1],(*it)[2]);
        colors->InsertNextTupleValue(green);
        vertices->InsertNextCell(1,pid);
        if(i < num-1){
            line->GetPointIds()->SetId(0,i);
            line->GetPointIds()->SetId(1,i+1);
            lines->InsertNextCell(line);
        }

    }

    Instantiate(polydata,vtkPolyData);
    polydata->SetPoints(points);
    polydata->SetVerts(vertices);
    polydata->SetLines(lines);
    polydata->GetPointData()->SetScalars(colors);

    Instantiate(cenmapper,vtkPolyDataMapper);
    cenmapper->SetInputData(polydata);

    m_lineactor->SetMapper(cenmapper);
    has_line = true;

    m_renderer->AddActor(m_lineactor);

    m_renWindow->Render();

}

void displayUtils::SetMainModelOn()
{
    if(!has_stl)  return;
    m_renderer->AddActor(m_stlactor);
    sliderWidget->EnabledOn();
    m_renderer->ResetCamera();

}

void displayUtils::SetMainModelOff()
{
    if(!has_stl)  return;
    m_renderer->RemoveActor(m_stlactor);
    sliderWidget->EnabledOff();
}

void displayUtils::SetLineModelOn()
{
    if(!has_line)  return;
    m_renderer->AddActor(m_lineactor);
    m_renderer->ResetCamera();
}

void displayUtils::SetLineModelOff()
{
    if(!has_line)  return;
    m_renderer->RemoveActor(m_lineactor);
}

void displayUtils::pointmode()
{
    if(m_renderer->GetActors()->GetNumberOfItems() != 0 && has_stl){
        m_stlactor->GetProperty()->SetRepresentationToPoints();
    }
}

void displayUtils::linemode()
{
    if(m_renderer->GetActors()->GetNumberOfItems() != 0 && has_stl){
        m_stlactor->GetProperty()->SetRepresentationToWireframe();
    }
}

void displayUtils::framemode()
{
    if(m_renderer->GetActors()->GetNumberOfItems() != 0 && has_stl){
        m_stlactor->GetProperty()->SetRepresentationToSurface();
    }
}

void displayUtils::TestDistanceTransform()
{
    m_centerline->getDistanceMap(GetRawFilename());
}

void displayUtils::GetCenterline(int option)
{
     double s[3],e[3];
     m_PointPickerInteractor->GetMarkedPoints(s,e);
     int tmp = option % 3;
     if(tmp == 0){
         m_centerline->Path_GradientDescent(GetRawFilename(),s,e);
     }else if(tmp == 1){
         m_centerline->Path_Thin3dImg(GetRawFilename(),s,e);
     }else{
         m_centerline->Path_Vornoimedial(GetRawFilename(),s,e);
     }
     std::cout << "complete centerline extraction!" << std::endl;
}


void displayUtils::OnRoam()
{
    // first calculate centerline if hasn't done
    if(m_centerline->GetCenterlinePointNums() == 0){
        m_centerline->getSignedDistanceMap_Sin(GetRawFilename());
        centerLnDis(m_centerline->getpoints());
    }
    m_currentRoamingIndex = 0;
    UpdateRoamingCamera();
}

void displayUtils::SetRoamingStep(int step)
{
    m_currentRoamingStep = step;
}

void displayUtils::RoamNext()
{
    m_currentRoamingIndex += m_currentRoamingStep;
    int indexLimit = m_centerline->GetCenterlinePointNums();
    if(m_currentRoamingIndex >= indexLimit ){
        m_currentRoamingIndex = std::max(0,indexLimit - 1);
    }
    UpdateRoamingCamera();
}

void displayUtils::RoamPrevious()
{
    m_currentRoamingIndex -= m_currentRoamingStep;
    if(m_currentRoamingIndex < 0){
        m_currentRoamingIndex = 0;
    }
    UpdateRoamingCamera();
}

void displayUtils::GetCurrentRoamingPoint(double p[])
{
    m_centerline->GetcenterlinePoint(m_currentRoamingIndex,p);
}

void displayUtils::UpdateRoamingCamera()
{
    double p[3];
    GetCurrentRoamingPoint(p);
    double fp[3];
    // to get current centerline point
    m_centerline->GetcenterlinePoint(m_currentRoamingIndex-m_currentRoamingStep/2,p);

    if (m_currentRoamingIndex - m_currentRoamingStep / 2 < 0){
        fp[0] = p[0],fp[1] = p[1], fp[2] = p[2];
    }

    double h[3];

    // to get current centerline point
    m_centerline->GetcenterlinePoint(m_currentRoamingIndex+m_currentRoamingStep/2,h);

    int indexLimit = m_centerline->GetCenterlinePointNums();
    if(m_currentRoamingIndex+m_currentRoamingStep/2 >= indexLimit){
        h[0] = p[0],h[1] = p[1],h[2] = p[2];
    }
    fp[0] = p[0] + h[0] - fp[0];
    fp[1] = p[1] + h[1] - fp[1];
    fp[2] = p[2] + h[2] - fp[2];

    m_renderer->GetActiveCamera()->SetPosition(p);
    m_renderer->GetActiveCamera()->SetFocalPoint(fp);

    m_light->SetPosition(m_renderer->GetActiveCamera()->GetPosition());
    m_light->SetFocalPoint(m_renderer->GetActiveCamera()->GetFocalPoint());

    m_renderer->Render();

}

void displayUtils::SetPointerPickEnabled(bool enabled)
{
    m_PointPickerInteractor->SetPickerEnabled(enabled);
}

void displayUtils::ShowCenterPoints(std::vector<Point3f> &CenterPoints)
{
    int num = CenterPoints.size();
    Instantiate(points,vtkPoints);
    Instantiate(vertices,vtkCellArray);
    Instantiate(lines,vtkCellArray);
    Instantiate(line,vtkLine);
    Instantiate(colors,vtkUnsignedCharArray);
    colors->SetNumberOfComponents(3);
    //green display each point.
    unsigned char blue[3] = {0,0,255};
    std::vector<Point3f>::const_iterator it = CenterPoints.begin();

    for(unsigned int i = 0;i < num && it != CenterPoints.end();i++,it++){
        vtkIdType pid[1];
        pid[0] = points->InsertNextPoint((*it).x,(*it).y,(*it).z);
        colors->InsertNextTupleValue(blue);
        vertices->InsertNextCell(1,pid);
        if(i < num-1){
            line->GetPointIds()->SetId(0,i);
            line->GetPointIds()->SetId(1,i+1);
            lines->InsertNextCell(line);
        }

    }

    Instantiate(polydata,vtkPolyData);
    polydata->SetPoints(points);
    polydata->SetVerts(vertices);
    polydata->SetLines(lines);
    polydata->GetPointData()->SetScalars(colors);

    Instantiate(cenmapper,vtkPolyDataMapper);
    cenmapper->SetInputData(polydata);

    m_lineactor->SetMapper(cenmapper);
    has_line = true;

    m_renderer->AddActor(m_lineactor);

    m_renWindow->Render();
}

void displayUtils::DrawCenterLine()
{
    ShowCenterPoints(m_centerline->CenterPoints);
}




void displayUtils::createSliderTool()
{

    vsp(sliderRep);
    sliderRep->SetMinimumValue(0.0);
    sliderRep->SetMaximumValue(1.0);
    sliderRep->SetValue(1.0);
    sliderRep->SetTitleText("Opacity");
    // Set color properties:
      // Change the color of the knob that slides
    sliderRep->GetSliderProperty()->SetColor(1,0,0);//red

      // Change the color of the text indicating what the slider controls
    sliderRep->GetTitleProperty()->SetColor(1,0,0);//red
//    sliderRep->SetTitleHeight(10);
      // Change the color of the text displaying the value
    sliderRep->GetLabelProperty()->SetColor(1,0,0);//red

      // Change the color of the knob when the mouse is held on it
    sliderRep->GetSelectedProperty()->SetColor(0,1,0);//green

      // Change the color of the bar
    sliderRep->GetTubeProperty()->SetColor(1,1,0);//yellow

      // Change the color of the ends of the bar
    sliderRep->GetCapProperty()->SetColor(1,1,0);//yellow
    sliderRep->GetPoint1Coordinate()->SetCoordinateSystemToDisplay();
    sliderRep->GetPoint1Coordinate()->SetValue(40 ,40);
    sliderRep->GetPoint2Coordinate()->SetCoordinateSystemToDisplay();
    sliderRep->GetPoint2Coordinate()->SetValue(160, 40);

    vsp(sliderWidget);
    sliderWidget->SetRepresentation(sliderRep);


    vsp(slidercallback);
    if(slidercallback->setBoundActor(vtkActor::SafeDownCast(m_stlactor))){
        sliderWidget->AddObserver(vtkCommand::InteractionEvent,slidercallback);
        sliderWidget->SetInteractor(m_iren);
        sliderWidget->SetAnimationModeToAnimate();
        sliderWidget->EnabledOff();
    }

}
