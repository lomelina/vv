/*=========================================================================
  Program:   vv                     http://www.creatis.insa-lyon.fr/rio/vv

  Authors belong to: 
  - University of LYON              http://www.universite-lyon.fr/
  - Léon Bérard cancer center       http://www.centreleonberard.fr
  - CREATIS CNRS laboratory         http://www.creatis.insa-lyon.fr

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the copyright notices for more information.

  It is distributed under dual licence

  - BSD        See included LICENSE.txt file
  - CeCILL-B   http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
===========================================================================**/
#ifndef clitkRegionGrowingGenericFilter_txx
#define clitkRegionGrowingGenericFilter_txx
#include <itkBinaryBallStructuringElement.h>
#include <itkConstShapedNeighborhoodIterator.h>

namespace clitk
{

  //-------------------------------------------------------------------
  // Update with the number of dimensions
  //-------------------------------------------------------------------
  template<unsigned int Dimension>
  void 
  RegionGrowingGenericFilter::UpdateWithDim(std::string PixelType)
  {
    if (m_Verbose) std::cout << "Image was detected to be "<<Dimension<<"D and "<< PixelType<<"..."<<std::endl;

    if(PixelType == "short"){  
      if (m_Verbose) std::cout << "Launching filter in "<< Dimension <<"D and signed short..." << std::endl;
      UpdateWithDimAndPixelType<Dimension, signed short>(); 
    }
    //    else if(PixelType == "unsigned_short"){  
    //       if (m_Verbose) std::cout  << "Launching filter in "<< Dimension <<"D and unsigned_short..." << std::endl;
    //       UpdateWithDimAndPixelType<Dimension, unsigned short>(); 
    //     }
    
    else if (PixelType == "unsigned_char"){ 
      if (m_Verbose) std::cout  << "Launching filter in "<< Dimension <<"D and unsigned_char..." << std::endl;
      UpdateWithDimAndPixelType<Dimension, unsigned char>();
    }
    
    //     else if (PixelType == "char"){ 
    //       if (m_Verbose) std::cout  << "Launching filter in "<< Dimension <<"D and signed_char..." << std::endl;
    //       UpdateWithDimAndPixelType<Dimension, signed char>();
    //     }
    else {
      if (m_Verbose) std::cout  << "Launching filter in "<< Dimension <<"D and float..." << std::endl;
      UpdateWithDimAndPixelType<Dimension, float>();
    }
  }


  //-------------------------------------------------------------------
  // Update with the number of dimensions and the pixeltype
  //-------------------------------------------------------------------
  template <unsigned int Dimension, class  PixelType> 
  void 
  RegionGrowingGenericFilter::UpdateWithDimAndPixelType()
  {

    // ImageTypes
    typedef itk::Image<PixelType, Dimension> InputImageType;
    typedef itk::Image<PixelType, Dimension> OutputImageType;
    
    // Read the input
    typedef itk::ImageFileReader<InputImageType> InputReaderType;
    typename InputReaderType::Pointer reader = InputReaderType::New();
    reader->SetFileName( m_InputFileName);
    reader->Update();
    typename InputImageType::Pointer input= reader->GetOutput();

    // Seed
    typedef typename  std::vector<typename InputImageType::IndexType> SeedsType;
    SeedsType seeds(1);
    if(m_ArgsInfo.seed_given==Dimension)
      for (unsigned int i=0; i<Dimension;i++)
	seeds[0][i]=m_ArgsInfo.seed_arg[i];
    
    else if ( m_ArgsInfo.seed_given==1)
      seeds[0].Fill(m_ArgsInfo.seed_arg[0]);
    
    else seeds[0].Fill(m_ArgsInfo.seed_arg[0]);
    if(m_Verbose)std::cout<<"Setting seed seeds to "<<seeds[0]<<"..."<<std::endl;

    if (m_ArgsInfo.seedRadius_given)
    {
      typedef itk::BinaryBallStructuringElement<PixelType, Dimension> BallType;
      typename BallType::RadiusType r;

      if (m_ArgsInfo.seedRadius_given == Dimension)
        for (unsigned i = 0; i < Dimension; i++)
          r[i] = m_ArgsInfo.seedRadius_arg[i];
      else
        r.Fill(m_ArgsInfo.seedRadius_arg[0]);

      BallType ball;
      ball.SetRadius(r);
      ball.CreateStructuringElement();

      typedef itk::ConstShapedNeighborhoodIterator<InputImageType> IteratorType;
      IteratorType it(ball.GetRadius(),
          input,
          input->GetLargestPossibleRegion());
#if ITK_VERSION_MAJOR < 4
      typename BallType::ConstIterator nit;
      unsigned idx = 0;
      for (nit = ball.Begin(); nit != ball.End(); ++nit, ++idx)
      {
        if (*nit)
        {
          it.ActivateOffset(it.GetOffset(idx));
        }
        else
        {
          it.DeactivateOffset(it.GetOffset(idx));
        }
      }
#else
      it.CreateActiveListFromNeighborhood(ball);
      it.NeedToUseBoundaryConditionOff();
#endif

      it.SetLocation(seeds[0]);
      for (typename IteratorType::ConstIterator i = it.Begin(); !i.IsAtEnd(); ++i)
      {
        typename InputImageType::IndexType id = seeds[0] + i.GetNeighborhoodOffset();
        if (id != seeds[0] && input->GetLargestPossibleRegion().IsInside(id))
          seeds.push_back(id);
      }
    }

    // Filter
    typedef itk::ImageToImageFilter<InputImageType, OutputImageType> ImageToImageFilterType;
    typename ImageToImageFilterType::Pointer filter;

    switch (m_ArgsInfo.type_arg)
      {
      case 0: {
		
	typedef itk::ConnectedThresholdImageFilter<InputImageType, OutputImageType> ImageFilterType;
	typename ImageFilterType::Pointer f= ImageFilterType::New();
	
	f->SetLower(m_ArgsInfo.lower_arg);
	f->SetUpper(m_ArgsInfo.upper_arg);
	f->SetReplaceValue(static_cast<PixelType>(m_ArgsInfo.pad_arg));
        for (typename SeedsType::const_iterator it = seeds.begin(); it != seeds.end(); ++it)
          f->AddSeed(*it);
	filter=f;
	if(m_Verbose)std::cout<<"Using the connected threshold image filter..."<<std::endl;

	break;
      }

      case 1: {
		
	typedef itk::NeighborhoodConnectedImageFilter<InputImageType, OutputImageType> ImageFilterType;
	typename ImageFilterType::Pointer f= ImageFilterType::New();
	
	// Radius
	typename  InputImageType::SizeType size;
	if(m_ArgsInfo.radius_given==Dimension)
	  for (unsigned int i=0; i<Dimension;i++)
	    size[i]=m_ArgsInfo.radius_arg[i];
	
	else if ( m_ArgsInfo.radius_given==1)
	  size.Fill(m_ArgsInfo.radius_arg[0]);
	
	else size.Fill(m_ArgsInfo.radius_arg[0]);
	if(m_Verbose)std::cout<<"Setting neighborhood radius to "<<size<<"..."<<std::endl;

	f->SetLower(m_ArgsInfo.lower_arg);
	f->SetUpper(m_ArgsInfo.upper_arg);
	f->SetReplaceValue(static_cast<PixelType>(m_ArgsInfo.pad_arg));
        for (typename SeedsType::const_iterator it = seeds.begin(); it != seeds.end(); ++it)
          f->AddSeed(*it);
	f->SetRadius(size);
	filter=f;
	if(m_Verbose)std::cout<<"Using the neighborhood threshold connected image filter..."<<std::endl;

	break;
      }

      case 2: {
	
	typedef itk::ConfidenceConnectedImageFilter<InputImageType, OutputImageType> ImageFilterType;
	typename ImageFilterType::Pointer f= ImageFilterType::New();
	
	// Radius
	typename  InputImageType::SizeType size;
	if(m_ArgsInfo.radius_given==Dimension)
	  for (unsigned int i=0; i<Dimension;i++)
	    size[i]=m_ArgsInfo.radius_arg[i];
	
	else if ( m_ArgsInfo.radius_given==1)
	  size.Fill(m_ArgsInfo.radius_arg[0]);
	
	else size.Fill(m_ArgsInfo.radius_arg[0]);
	if(m_Verbose)std::cout<<"Setting neighborhood radius to "<<size<<"..."<<std::endl;

	f->SetMultiplier( m_ArgsInfo.multiplier_arg );
	f->SetNumberOfIterations( m_ArgsInfo.multiplier_arg );
        for (typename SeedsType::const_iterator it = seeds.begin(); it != seeds.end(); ++it)
          f->AddSeed(*it);
	f->SetNumberOfIterations( m_ArgsInfo.iter_arg);
	f->SetReplaceValue(static_cast<PixelType>(m_ArgsInfo.pad_arg));
	f->SetInitialNeighborhoodRadius(size[0]);
	filter=f;
	if(m_Verbose)std::cout<<"Using the confidence threshold connected image filter..."<<std::endl;
	
	break;
      }

      case 3: {
		
	typedef clitk::LocallyAdaptiveThresholdConnectedImageFilter<InputImageType, OutputImageType> ImageFilterType;
	typename ImageFilterType::Pointer f= ImageFilterType::New();
	
	// Radius
	typename  InputImageType::SizeType size;
	if(m_ArgsInfo.radius_given==Dimension)
	  for (unsigned int i=0; i<Dimension;i++)
	    size[i]=m_ArgsInfo.radius_arg[i];
	else size.Fill(m_ArgsInfo.radius_arg[0]);
	if(m_Verbose)std::cout<<"Setting neighborhood radius to "<<size<<"..."<<std::endl;

	// params
	f->SetLower(m_ArgsInfo.lower_arg);
	f->SetUpper(m_ArgsInfo.upper_arg);
	f->SetLowerBorderIsGiven(m_ArgsInfo.adaptLower_flag);
	f->SetLowerBorderIsGiven(m_ArgsInfo.adaptUpper_flag);
	f->SetReplaceValue(static_cast<PixelType>(m_ArgsInfo.pad_arg));
	f->SetMultiplier(m_ArgsInfo.multiplier_arg);
	f->SetMaximumSDIsGiven(m_ArgsInfo.maxSD_given);
	if (m_ArgsInfo.maxSD_given) f->SetMaximumSD(m_ArgsInfo.maxSD_arg);
        for (typename SeedsType::const_iterator it = seeds.begin(); it != seeds.end(); ++it)
          f->AddSeed(*it);
	f->SetRadius(size);
	filter=f;
	if(m_Verbose)std::cout<<"Using the locally adaptive threshold connected image filter..."<<std::endl;

	break;
      }
  
      case 4: {
		
	typedef clitk::ExplosionControlledThresholdConnectedImageFilter<InputImageType, OutputImageType> ImageFilterType;
	typename ImageFilterType::Pointer f= ImageFilterType::New();
	
	// 	// Radius
	// 	typename  InputImageType::SizeType size;
	// 	if(m_ArgsInfo.radius_given==Dimension)
	// 	  for (unsigned int i=0; i<Dimension;i++)
	// 	    size[i]=m_ArgsInfo.radius_arg[i];
	// 	else size.Fill(m_ArgsInfo.radius_arg[0]);
	// 	if(m_Verbose)std::cout<<"Setting neighborhood radius to "<<size<<"..."<<std::endl;
	
	// params
	f->SetVerbose(m_ArgsInfo.verbose_flag);
	f->SetLower(m_ArgsInfo.lower_arg);
	f->SetUpper(m_ArgsInfo.upper_arg);
	f->SetMinimumLowerThreshold(m_ArgsInfo.minLower_arg);
	f->SetMaximumUpperThreshold(m_ArgsInfo.maxUpper_arg);
	f->SetAdaptLowerBorder(m_ArgsInfo.adaptLower_flag);
	f->SetAdaptUpperBorder(m_ArgsInfo.adaptUpper_flag);
	f->SetReplaceValue(static_cast<PixelType>(m_ArgsInfo.pad_arg));
	f->SetMultiplier(m_ArgsInfo.multiplier_arg);
	f->SetThresholdStepSize(m_ArgsInfo.step_arg);
	f->SetMinimumThresholdStepSize(m_ArgsInfo.minStep_arg);
	f->SetFullyConnected(m_ArgsInfo.full_flag);
        for (typename SeedsType::const_iterator it = seeds.begin(); it != seeds.end(); ++it)
          f->AddSeed(*it);
	filter=f;
	if(m_Verbose)std::cout<<"Using the explosion controlled threshold connected image filter..."<<std::endl;

	break;
      }
  
    }


    filter->SetInput(input);
    filter->Update();
    typename OutputImageType::Pointer output=filter->GetOutput();

    // Output
    typedef itk::ImageFileWriter<OutputImageType> WriterType;
    typename WriterType::Pointer writer = WriterType::New();
    writer->SetFileName(m_ArgsInfo.output_arg);
    writer->SetInput(output);
    writer->Update();
  }

}//end clitk
 
#endif //#define clitkRegionGrowingGenericFilter_txx