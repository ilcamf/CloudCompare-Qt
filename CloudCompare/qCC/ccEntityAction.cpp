//##########################################################################
//#                                                                        #
//#                              CLOUDCOMPARE                              #
//#                                                                        #
//#  This program is free software; you can redistribute it and/or modify  #
//#  it under the terms of the GNU General Public License as published by  #
//#  the Free Software Foundation; version 2 or later of the License.      #
//#                                                                        #
//#  This program is distributed in the hope that it will be useful,       #
//#  but WITHOUT ANY WARRANTY; without even the implied warranty of        #
//#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the          #
//#  GNU General Public License for more details.                          #
//#                                                                        #
//#          COPYRIGHT: CloudCompare project                               #
//#                                                                        #
//##########################################################################

//Qt
#include <QColorDialog>
#include <QElapsedTimer>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>

//CCLib
#include <NormalDistribution.h>
#include <ScalarFieldTools.h>
#include <StatisticalTestingTools.h>
#include <WeibullDistribution.h>

//qCC_db
#include "ccColorScalesManager.h"
#include "ccFacet.h"
#include "ccGenericPrimitive.h"
#include "ccOctreeProxy.h"

//qCC_gl
#include "ccGuiParameters.h"

//Local
#include "ccAskTwoDoubleValuesDlg.h"
#include "ccColorGradientDlg.h"
#include "ccColorLevelsDlg.h"
#include "ccComputeOctreeDlg.h"
#include "ccExportCoordToSFDlg.h"
#include "ccNormalComputationDlg.h"
#include "ccPickOneElementDlg.h"
#include "ccProgressDialog.h"
#include "ccScalarFieldArithmeticsDlg.h"
#include "ccScalarFieldFromColorDlg.h"
#include "ccStatisticalTestDlg.h"

#include "ccCommon.h"
#include "ccConsole.h"
#include "ccEntityAction.h"
#include "ccHistogramWindow.h"
#include "ccLibAlgorithms.h"
#include "ccUtils.h"

// This is included only for temporarily removing an object from the tree.
//	TODO figure out a cleaner way to do this without having to include all of mainwindow.h
#include "mainwindow.h"


namespace ccEntityAction
{
	static QString GetFirstAvailableSFName(const ccPointCloud* cloud, const QString& baseName)
	{
		if (cloud == nullptr)
		{
			Q_ASSERT(false);
			return QString();
		}
		
		QString name = baseName;
		int tries = 0;
		
		while (cloud->getScalarFieldIndexByName(qPrintable(name)) >= 0 || tries > 99)
			name = QString("%1 #%2").arg(baseName).arg(++tries);
		
		if (tries > 99)
			return QString();
		
		return name;
	}
	
	//////////
	// Colours
	bool setColor(ccHObject::Container selectedEntities, bool colorize, QWidget *parent)
	{
		QColor colour = QColorDialog::getColor(Qt::white, parent);
		
		if (!colour.isValid())
			return false;
		
		while (!selectedEntities.empty())
		{
			ccHObject* ent = selectedEntities.back();
			selectedEntities.pop_back();
			if (ent->isA(CC_TYPES::HIERARCHY_OBJECT))
			{
				//automatically parse a group's children set
				for (unsigned i=0; i<ent->getChildrenNumber(); ++i)
					selectedEntities.push_back(ent->getChild(i));
			}
			else if (ent->isA(CC_TYPES::POINT_CLOUD) || ent->isA(CC_TYPES::MESH))
			{
				ccPointCloud* cloud = 0;
				if (ent->isA(CC_TYPES::POINT_CLOUD))
				{
					cloud = static_cast<ccPointCloud*>(ent);
				}
				else
				{
					ccMesh* mesh = static_cast<ccMesh*>(ent);
					ccGenericPointCloud* vertices = mesh->getAssociatedCloud();
					if (	!vertices
							||	!vertices->isA(CC_TYPES::POINT_CLOUD)
							||	(vertices->isLocked() && !mesh->isAncestorOf(vertices)) )
					{
						ccLog::Warning(QString("[SetColor] Can't set color for mesh '%1' (vertices are not accessible)").arg(ent->getName()));
						continue;
					}
					
					cloud = static_cast<ccPointCloud*>(vertices);
				}
				
				if (colorize)
				{
					cloud->colorize(static_cast<float>(colour.redF()),
									static_cast<float>(colour.greenF()),
									static_cast<float>(colour.blueF()) );
				}
				else
				{
					cloud->setRGBColor(	static_cast<ColorCompType>(colour.red()),
										static_cast<ColorCompType>(colour.green()),
										static_cast<ColorCompType>(colour.blue()) );
				}
				cloud->showColors(true);
				cloud->showSF(false); //just in case
				cloud->prepareDisplayForRefresh();
				
				if (ent != cloud)
				{
					ent->showColors(true);
				}
				else if (cloud->getParent() && cloud->getParent()->isKindOf(CC_TYPES::MESH))
				{
					cloud->getParent()->showColors(true);
					cloud->getParent()->showSF(false); //just in case
				}

			}
			else if (ent->isKindOf(CC_TYPES::PRIMITIVE))
			{
				ccGenericPrimitive* prim = ccHObjectCaster::ToPrimitive(ent);
				ccColor::Rgb col(	static_cast<ColorCompType>(colour.red()),
										static_cast<ColorCompType>(colour.green()),
										static_cast<ColorCompType>(colour.blue()) );
				prim->setColor(col);
				ent->showColors(true);
				ent->showSF(false); //just in case
				ent->prepareDisplayForRefresh();
			}
			else if (ent->isA(CC_TYPES::POLY_LINE))
			{
				ccPolyline* poly = ccHObjectCaster::ToPolyline(ent);
				ccColor::Rgb col(	static_cast<ColorCompType>(colour.red()),
									static_cast<ColorCompType>(colour.green()),
									static_cast<ColorCompType>(colour.blue()) );
				poly->setColor(col);
				ent->showColors(true);
				ent->showSF(false); //just in case
				ent->prepareDisplayForRefresh();
			}
			else if (ent->isA(CC_TYPES::FACET))
			{
				ccFacet* facet = ccHObjectCaster::ToFacet(ent);
				ccColor::Rgb col(	static_cast<ColorCompType>(colour.red()),
									static_cast<ColorCompType>(colour.green()),
									static_cast<ColorCompType>(colour.blue()) );
				facet->setColor(col);
				ent->showColors(true);
				ent->showSF(false); //just in case
				ent->prepareDisplayForRefresh();
			}
			else
			{
				ccLog::Warning(QString("[SetColor] Can't change color of entity '%1'").arg(ent->getName()));
			}
		}
		
		return true;
	}
	
	bool	rgbToGreyScale(const ccHObject::Container &selectedEntities)
	{
		size_t selNum = selectedEntities.size();
		
		for (size_t i=0; i<selNum; ++i)
		{
			ccHObject* ent = selectedEntities[i];
			
			bool lockedVertices = false;
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent, &lockedVertices);
			if (lockedVertices)
			{
				ccUtils::DisplayLockedVerticesWarning(ent->getName(), selNum == 1);
				continue;
			}
			
			if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD))
			{
				ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
				if (pc->hasColors())
				{
					pc->convertRGBToGreyScale();
					pc->showColors(true);
					pc->showSF(false); //just in case
					pc->prepareDisplayForRefresh();
				}
			}
		}
		
		return true;
	}
	
	bool setColorGradient(const ccHObject::Container &selectedEntities, QWidget *parent)
	{	
		ccColorGradientDlg dlg(parent);
		if (!dlg.exec())
			return false;
		
		unsigned char dim = dlg.getDimension();
		ccColorGradientDlg::GradientType ramp = dlg.getType();
		
		ccColorScale::Shared colorScale(0);
		if (ramp == ccColorGradientDlg::Default)
		{
			colorScale = ccColorScalesManager::GetDefaultScale();
		}
		else if (ramp == ccColorGradientDlg::TwoColors)
		{
			colorScale = ccColorScale::Create("Temp scale");
			QColor first,second;
			dlg.getColors(first,second);
			colorScale->insert(ccColorScaleElement(0.0, first), false);
			colorScale->insert(ccColorScaleElement(1.0, second), true);
		}
		
		Q_ASSERT(colorScale || ramp == ccColorGradientDlg::Banding);
		
		const double frequency = dlg.getBandingFrequency();
		
		size_t selNum = selectedEntities.size();
		for (size_t i = 0; i < selNum; ++i)
		{
			ccHObject* ent = selectedEntities[i];
			
			bool lockedVertices = false;
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
			if (lockedVertices)
			{
				ccUtils::DisplayLockedVerticesWarning(ent->getName(),selNum == 1);
				continue;
			}
			
			if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) // TODO
			{
				ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
				
				bool success = false;
				if (ramp == ccColorGradientDlg::Banding)
					success = pc->setRGBColorByBanding(dim, frequency);
				else
					success = pc->setRGBColorByHeight(dim, colorScale);
				
				if (success)
				{
					ent->showColors(true);
					ent->showSF(false); //just in case
					ent->prepareDisplayForRefresh();
				}
			}
		}
		
		return true;
	}
	
	bool	changeColorLevels(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		if (selectedEntities.size() != 1)
		{
			ccConsole::Error("Select one and only one colored cloud or mesh!");
			return false;
		}
		
		bool lockedVertices;
		ccPointCloud* pointCloud = ccHObjectCaster::ToPointCloud(selectedEntities[0], &lockedVertices);
		if (!pointCloud || lockedVertices)
		{
			if (lockedVertices)
				ccUtils::DisplayLockedVerticesWarning(pointCloud->getName(), true);
			return false;
		}
		
		if (!pointCloud->hasColors())
		{
			ccConsole::Error("Selected entity has no colors!");
			return false;
		}
		
		ccColorLevelsDlg dlg(parent, pointCloud);
		dlg.exec();
		
		return true;
	}
	
	bool	interpolateColors(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		if (selectedEntities.size() != 2)
		{
			ccConsole::Error("Select 2 entities (clouds or meshes)!");
			return false;
		}
		
		ccHObject* ent1 = selectedEntities[0];
		ccHObject* ent2 = selectedEntities[1];
		
		ccGenericPointCloud* cloud1 = ccHObjectCaster::ToGenericPointCloud(ent1);
		ccGenericPointCloud* cloud2 = ccHObjectCaster::ToGenericPointCloud(ent2);
		
		if (!cloud1 || !cloud2)
		{
			ccConsole::Error("Select 2 entities (clouds or meshes)!");
			return false;
		}
		
		if (!cloud1->hasColors() && !cloud2->hasColors())
		{
			ccConsole::Error("None of the selected entities has per-point or per-vertex colors!");
			return false;
		}
		else if (cloud1->hasColors() && cloud2->hasColors())
		{
			ccConsole::Error("Both entities have colors! Remove the colors on the entity you wish to import the colors to!");
			return false;
		}
		
		ccGenericPointCloud* source = cloud1;
		ccGenericPointCloud* dest = cloud2;
		
		if ( cloud2->hasColors())
		{
			std::swap(source, dest);
			std::swap(cloud1, cloud2);
			std::swap(ent1, ent2);
		}
		
		if (!dest->isA(CC_TYPES::POINT_CLOUD))
		{
			ccConsole::Error("Destination cloud (or vertices) must be a real point cloud!");
			return false;
		}
		
		bool ok = false;
		unsigned char defaultLevel = 7;
		int value = QInputDialog::getInt(	parent,
											"Interpolate colors", "Octree level",
											defaultLevel,
											1, CCLib::DgmOctree::MAX_OCTREE_LEVEL,
											1,
											&ok);
		if (!ok)
			return false;
		
		defaultLevel = static_cast<unsigned char>(value);
		
		ccProgressDialog pDlg(true, parent);
		
		if (static_cast<ccPointCloud*>(dest)->interpolateColorsFrom(source, &pDlg, defaultLevel))
		{
			ent2->showColors(true);
			ent2->showSF(false); //just in case
		}
		else
		{
			ccConsole::Error("An error occurred! (see console)");
		}
		
		ent2->prepareDisplayForRefresh_recursive();
		
		return true;
	}
	
	bool	convertTextureToColor(ccHObject::Container selectedEntities, QWidget *parent)
	{	
		for (size_t i=0; i<selectedEntities.size(); ++i)
		{
			ccHObject* ent = selectedEntities[i];
			if (ent->isA(CC_TYPES::MESH)/*|| ent->isKindOf(CC_TYPES::PRIMITIVE)*/) //TODO
			{
				ccMesh* mesh = ccHObjectCaster::ToMesh(ent);
				Q_ASSERT(mesh);
				
				if (!mesh->hasMaterials())
				{
					ccLog::Warning(QString("[convertTextureToColor] Mesh '%1' has no material/texture!").arg(mesh->getName()));
					continue;
				}
				else
				{
					if (QMessageBox::warning( parent,
											  "Mesh already has colors",
											  QString("Mesh '%1' already has colors! Overwrite them?").arg(mesh->getName()),
											  QMessageBox::Yes | QMessageBox::No,
											  QMessageBox::No ) != QMessageBox::Yes)
					{
						continue;
					}
					
					//ColorCompType C[3]={MAX_COLOR_COMP,MAX_COLOR_COMP,MAX_COLOR_COMP};
					//mesh->getColorFromMaterial(triIndex,*P,C,withRGB);
					//cloud->addRGBColor(C);
					if (mesh->convertMaterialsToVertexColors())
					{
						mesh->showColors(true);
						mesh->showSF(false); //just in case
						mesh->showMaterials(false);
						mesh->prepareDisplayForRefresh_recursive();
					}
					else
					{
						ccLog::Warning(QString("[convertTextureToColor] Failed to convert texture on mesh '%1'!").arg(mesh->getName()));
					}
				}
			}
		}
		
		return true;
	}
	
	//////////
	// Scalar Fields
	
	bool sfGaussianFilter(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		size_t selNum = selectedEntities.size();
		if (selNum == 0)
			return false;
		
		double sigma = ccLibAlgorithms::GetDefaultCloudKernelSize(selectedEntities);
		if (sigma < 0.0)
		{
			ccConsole::Error("No eligible point cloud in selection!");
			return false;
		}
		
		bool ok = false;
		sigma = QInputDialog::getDouble(parent,
										"Gaussian filter",
										"sigma:",
										sigma,
										DBL_MIN,
										1.0e9,
										8,
										&ok);
		if (!ok)
			return false;
		
		ccProgressDialog pDlg(true, parent);
		pDlg.setAutoClose(false);

		for (size_t i = 0; i<selNum; ++i)
		{
			bool lockedVertices = false;
			ccHObject* ent = selectedEntities[i];
			ccPointCloud* pc = ccHObjectCaster::ToPointCloud(ent, &lockedVertices);
			if (!pc || lockedVertices)
			{
				ccUtils::DisplayLockedVerticesWarning(ent->getName(), selNum == 1);
				continue;
			}
			
			//la methode est activee sur le champ scalaire affiche
			CCLib::ScalarField* sf = pc->getCurrentDisplayedScalarField();
			if (sf != nullptr)
			{
				//on met en lecture (OUT) le champ scalaire actuellement affiche
				int outSfIdx = pc->getCurrentDisplayedScalarFieldIndex();
				Q_ASSERT(outSfIdx >= 0);
				
				pc->setCurrentOutScalarField(outSfIdx);
				CCLib::ScalarField* outSF = pc->getCurrentOutScalarField();
				Q_ASSERT(sf != nullptr);
				
				QString sfName = QString("%1.smooth(%2)").arg(outSF->getName()).arg(sigma);
				int sfIdx = pc->getScalarFieldIndexByName(qPrintable(sfName));
				if (sfIdx < 0)
					sfIdx = pc->addScalarField(qPrintable(sfName)); //output SF has same type as input SF
				if (sfIdx >= 0)
					pc->setCurrentInScalarField(sfIdx);
				else
				{
					ccConsole::Error(QString("Failed to create scalar field for cloud '%1' (not enough memory?)").arg(pc->getName()));
					continue;
				}
				
				ccOctree::Shared octree = pc->getOctree();
				if (!octree)
				{
					octree = pc->computeOctree(&pDlg);
					if (!octree)
					{
						ccConsole::Error(QString("Couldn't compute octree for cloud '%1'!").arg(pc->getName()));
						continue;
					}
				}
				
				if (octree)
				{
					QElapsedTimer eTimer;
					eTimer.start();
					CCLib::ScalarFieldTools::applyScalarFieldGaussianFilter(static_cast<PointCoordinateType>(sigma),
																			pc,
																			-1,
																			&pDlg,
																			octree.data());
					
					ccConsole::Print("[GaussianFilter] Timing: %3.2f s.", static_cast<double>(eTimer.elapsed()) / 1000.0);
					pc->setCurrentDisplayedScalarField(sfIdx);
					pc->showSF(sfIdx >= 0);
					sf = pc->getCurrentDisplayedScalarField();
					if (sf)
						sf->computeMinAndMax();
					pc->prepareDisplayForRefresh_recursive();
				}
				else
				{
					ccConsole::Error(QString("Failed to compute entity [%1] octree! (not enough memory?)").arg(pc->getName()));
				}
			}
			else
			{
				ccConsole::Warning(QString("Entity [%1] has no active scalar field!").arg(pc->getName()));
			}
		}
		
		return true;
	}
	
	bool	sfBilateralFilter(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		size_t selNum = selectedEntities.size();
		if (selNum == 0)
			return false;
		
		double sigma = ccLibAlgorithms::GetDefaultCloudKernelSize(selectedEntities);
		if (sigma < 0.0)
		{
			ccConsole::Error("No eligible point cloud in selection!");
			return false;
		}
		
		//estimate a good value for scalar field sigma, based on the first cloud
		//and its displayed scalar field
		ccPointCloud* pc_test = ccHObjectCaster::ToPointCloud(selectedEntities[0]);
		CCLib::ScalarField* sf_test = pc_test->getCurrentDisplayedScalarField();
		ScalarType range = sf_test->getMax() - sf_test->getMin();
		double scalarFieldSigma = range / 4; // using 1/4 of total range
		
		
		ccAskTwoDoubleValuesDlg dlg("Spatial sigma",
									"Scalar sigma",
									DBL_MIN,
									1.0e9,
									sigma,
									scalarFieldSigma,
									8,
									nullptr,
									parent);
		
		dlg.doubleSpinBox1->setStatusTip("3*sigma = 98% attenuation");
		dlg.doubleSpinBox2->setStatusTip("Scalar field's sigma controls how much the filter behaves as a Gaussian Filter\n sigma at +inf uses the whole range of scalars ");
		if (!dlg.exec())
			return false;
		
		//get values
		sigma = dlg.doubleSpinBox1->value();
		scalarFieldSigma = dlg.doubleSpinBox2->value();
		
		ccProgressDialog pDlg(true, parent);
		pDlg.setAutoClose(false);

		for (size_t i = 0; i<selNum; ++i)
		{
			bool lockedVertices = false;
			ccHObject* ent = selectedEntities[i];
			ccPointCloud* pc = ccHObjectCaster::ToPointCloud(ent, &lockedVertices);
			if (!pc || lockedVertices)
			{
				ccUtils::DisplayLockedVerticesWarning(ent->getName(), selNum == 1);
				continue;
			}
			
			//the algorithm will use the currently displayed SF
			CCLib::ScalarField* sf = pc->getCurrentDisplayedScalarField();
			if (sf != nullptr)
			{
				//we set the displayed SF as "OUT" SF
				int outSfIdx = pc->getCurrentDisplayedScalarFieldIndex();
				Q_ASSERT(outSfIdx >= 0);
				
				pc->setCurrentOutScalarField(outSfIdx);
				CCLib::ScalarField* outSF = pc->getCurrentOutScalarField();
				Q_ASSERT(outSF != nullptr);
				
				QString sfName = QString("%1.bilsmooth(%2,%3)").arg(outSF->getName()).arg(sigma).arg(scalarFieldSigma);
				int sfIdx = pc->getScalarFieldIndexByName(qPrintable(sfName));
				if (sfIdx < 0)
					sfIdx = pc->addScalarField(qPrintable(sfName)); //output SF has same type as input SF
				if (sfIdx >= 0)
					pc->setCurrentInScalarField(sfIdx);
				else
				{
					ccConsole::Error(QString("Failed to create scalar field for cloud '%1' (not enough memory?)").arg(pc->getName()));
					continue;
				}
				
				ccOctree::Shared octree = pc->getOctree();
				if (!octree)
				{
					octree = pc->computeOctree(&pDlg);
					if (!octree)
					{
						ccConsole::Error(QString("Couldn't compute octree for cloud '%1'!").arg(pc->getName()));
						continue;
					}
				}
				
				Q_ASSERT(octree != 0);
				{
					QElapsedTimer eTimer;
					eTimer.start();
					
					CCLib::ScalarFieldTools::applyScalarFieldGaussianFilter(static_cast<PointCoordinateType>(sigma),
																			pc,
																			static_cast<PointCoordinateType>(scalarFieldSigma),
																			&pDlg,
																			octree.data());
					
					ccConsole::Print("[BilateralFilter] Timing: %3.2f s.", eTimer.elapsed() / 1000.0);
					pc->setCurrentDisplayedScalarField(sfIdx);
					pc->showSF(sfIdx >= 0);
					sf = pc->getCurrentDisplayedScalarField();
					if (sf)
						sf->computeMinAndMax();
					pc->prepareDisplayForRefresh_recursive();
				}
			}
			else
			{
				ccConsole::Warning(QString("Entity [%1] has no active scalar field!").arg(pc->getName()));
			}
		}
		
		return true;
	}
	
	bool sfConvertToRGB(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		//we first ask the user if the SF colors should be mixed with existing colors
		bool mixWithExistingColors = false;
		
		QMessageBox::StandardButton answer = QMessageBox::warning(	parent,
																	"Scalar Field to RGB",
																	"Mix with existing colors (if any)?",
																	QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
																	QMessageBox::Yes );
		if (answer == QMessageBox::Yes)
			mixWithExistingColors = true;
		else if (answer == QMessageBox::Cancel)
			return false;
		
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccGenericPointCloud* cloud = nullptr;
			ccHObject* ent = selectedEntities[i];
			
			bool lockedVertices = false;
			cloud = ccHObjectCaster::ToPointCloud(ent, &lockedVertices);
			if (lockedVertices)
			{
				ccUtils::DisplayLockedVerticesWarning(ent->getName(), selNum == 1);
				continue;
			}
			if (cloud != nullptr) //TODO
			{
				ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
				//if there is no displayed SF --> nothing to do!
				if (pc->getCurrentDisplayedScalarField())
				{
					if (pc->setRGBColorWithCurrentScalarField(mixWithExistingColors))
					{
						ent->showColors(true);
						ent->showSF(false); //just in case
					}
				}
				
				cloud->prepareDisplayForRefresh_recursive();
			}
		}
		
		return true;
	}
	
	bool	sfConvertToRandomRGB(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		static int s_randomColorsNumber = 256;
		
		bool ok = false;
		s_randomColorsNumber = QInputDialog::getInt(parent,
													"Random colors",
													"Number of random colors (will be regularly sampled over the SF interval):",
													s_randomColorsNumber,
													2,
													INT_MAX,
													16,
													&ok);
		if (!ok)
			return false;
		Q_ASSERT(s_randomColorsNumber > 1);
		
		ColorsTableType* randomColors = new ColorsTableType;
		if (!randomColors->reserve(static_cast<unsigned>(s_randomColorsNumber)))
		{
			ccConsole::Error("Not enough memory!");
			return false;
		}
		
		//generate random colors
		for (int i=0; i<s_randomColorsNumber; ++i)
		{
			ccColor::Rgb col = ccColor::Generator::Random();
			randomColors->addElement(col.rgb);
		}
		
		//apply random colors
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccGenericPointCloud* cloud = nullptr;
			ccHObject* ent = selectedEntities[i];
			
			bool lockedVertices = false;
			cloud = ccHObjectCaster::ToPointCloud(ent,&lockedVertices);
			if (lockedVertices)
			{
				ccUtils::DisplayLockedVerticesWarning(ent->getName(),selNum == 1);
				continue;
			}
			if (cloud != nullptr) //TODO
			{
				ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
				ccScalarField* sf = pc->getCurrentDisplayedScalarField();
				//if there is no displayed SF --> nothing to do!
				if (sf && sf->currentSize() >= pc->size())
				{
					if (!pc->resizeTheRGBTable(false))
					{
						ccConsole::Error("Not enough memory!");
						break;
					}
					else
					{
						ScalarType minSF = sf->getMin();
						ScalarType maxSF = sf->getMax();
						
						ScalarType step = (maxSF - minSF) / (s_randomColorsNumber - 1);
						if (step == 0)
							step = static_cast<ScalarType>(1.0);
						
						for (unsigned i = 0; i < pc->size(); ++i)
						{
							ScalarType val = sf->getValue(i);
							unsigned colIndex = static_cast<unsigned>((val - minSF) / step);
							if (colIndex == s_randomColorsNumber)
								--colIndex;
							
							pc->setPointColor(i, randomColors->getValue(colIndex));
						}
						
						pc->showColors(true);
						pc->showSF(false); //just in case
					}
				}
				
				cloud->prepareDisplayForRefresh_recursive();
			}
		}
		
		return true;
	}
	
	bool sfRename(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccGenericPointCloud* cloud = ccHObjectCaster::ToPointCloud(selectedEntities[i]);
			if (cloud != nullptr) //TODO
			{
				ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
				ccScalarField* sf = pc->getCurrentDisplayedScalarField();
				//if there is no displayed SF --> nothing to do!
				if (sf == nullptr)
				{
					ccConsole::Warning(QString("Cloud %1 has no displayed scalar field!").arg(pc->getName()));
				}
				else
				{
					const char* sfName = sf->getName();
					bool ok = false;
					QString newName = QInputDialog::getText(parent,
															"SF name",
															"name:",
															QLineEdit::Normal,
															QString(sfName ? sfName : "unknown"),
															&ok);
					if (ok)
						sf->setName(qPrintable(newName));
				}
			}
		}
		
		return true;
	}
	
	bool	sfAddIdField(const ccHObject::Container &selectedEntities)
	{
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccGenericPointCloud* cloud = ccHObjectCaster::ToPointCloud(selectedEntities[i]);
			if (cloud != nullptr) //TODO
			{
				ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
				
				int sfIdx = pc->getScalarFieldIndexByName(CC_DEFAULT_ID_SF_NAME);
				if (sfIdx < 0)
					sfIdx = pc->addScalarField(CC_DEFAULT_ID_SF_NAME);
				if (sfIdx < 0)
				{
					ccLog::Warning("Not enough memory!");
					return false;
				}
				
				CCLib::ScalarField* sf = pc->getScalarField(sfIdx);
				Q_ASSERT(sf->currentSize() == pc->size());
				
				for (unsigned j=0 ; j<cloud->size(); j++)
				{
					ScalarType idValue = static_cast<ScalarType>(j);
					sf->setValue(j, idValue);
				}
				
				sf->computeMinAndMax();
				pc->setCurrentDisplayedScalarField(sfIdx);
				pc->showSF(true);
				pc->prepareDisplayForRefresh();
			}
		}
		
		return true;
	}
	
	bool	sfSetAsCoord(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		ccExportCoordToSFDlg ectsDlg(parent);
		ectsDlg.warningLabel->setVisible(false);
		ectsDlg.setWindowTitle("Export SF to coordinate(s)");
		
		if (!ectsDlg.exec())
			return false;
		
		bool exportDim[3] = { ectsDlg.exportX(), ectsDlg.exportY(), ectsDlg.exportZ() };
		if (!exportDim[0] && !exportDim[1] && !exportDim[2]) //nothing to do?!
			return false;
		
		//for each selected cloud (or vertices set)
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(selectedEntities[i]);
			if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD))
			{
				ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
				
				ccScalarField* sf = pc->getCurrentDisplayedScalarField();
				if (sf != nullptr)
				{
					unsigned ptsCount = pc->size();
					bool hasDefaultValueForNaN = false;
					ScalarType defaultValueForNaN = sf->getMin();
					
					for (unsigned i=0; i<ptsCount; ++i)
					{
						ScalarType s = sf->getValue(i);
						
						//handle NaN values
						if (!CCLib::ScalarField::ValidValue(s))
						{
							if (!hasDefaultValueForNaN)
							{
								bool ok = false;
								double out = QInputDialog::getDouble(	parent,
																		"SF --> coordinate",
																		"Enter the coordinate equivalent for NaN values:",
																		defaultValueForNaN,
																		-1.0e9,
																		1.0e9,
																		6,
																		&ok);
								if (ok)
									defaultValueForNaN = static_cast<ScalarType>(out);
								else
									ccLog::Warning("[SetSFAsCoord] By default the coordinate equivalent for NaN values will be the minimum SF value");
								hasDefaultValueForNaN = true;
							}
							s = defaultValueForNaN;
						}
						
						CCVector3* P = const_cast<CCVector3*>(pc->getPoint(i));
						
						//test each dimension
						if (exportDim[0])
							P->x = s;
						if (exportDim[1])
							P->y = s;
						if (exportDim[2])
							P->z = s;
					}
					
					pc->invalidateBoundingBox();
				}
			}
		}
		
		return true;
	}
	
	bool	exportCoordToSF(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		ccExportCoordToSFDlg ectsDlg(parent);
		
		if (!ectsDlg.exec())
			return false;
		
		bool exportDim[3] = { ectsDlg.exportX(), ectsDlg.exportY(), ectsDlg.exportZ() };
		
		if (!exportDim[0] && !exportDim[1] && !exportDim[2]) //nothing to do?!
			return false;
		
		const QString defaultSFName[3] = {"Coord. X", "Coord. Y", "Coord. Z"};
		
		//for each selected cloud (or vertices set)
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccHObject* entity = selectedEntities[i];
			ccPointCloud* pc = ccHObjectCaster::ToPointCloud(entity);
			if (pc == nullptr)
			{
				// TODO do something with error?
				continue;
			}
			
			unsigned ptsCount = pc->size();
			
			//test each dimension
			for (unsigned d=0; d<3; ++d)
			{
				if (!exportDim[d])
					continue;
				
				int sfIndex = pc->getScalarFieldIndexByName(qPrintable(defaultSFName[d]));
				if (sfIndex < 0)
					sfIndex = pc->addScalarField(qPrintable(defaultSFName[d]));
				if (sfIndex < 0)
				{
					ccLog::Error("Not enough memory!");
					i = selNum;
					break;
				}
				
				CCLib::ScalarField* sf = pc->getScalarField(sfIndex);
				Q_ASSERT(sf && sf->currentSize() == ptsCount);
				if (sf != nullptr)
				{
					for (unsigned k=0; k<ptsCount; ++k)
					{
						ScalarType s = static_cast<ScalarType>(pc->getPoint(k)->u[d]);
						sf->setValue(k,s);
					}
					sf->computeMinAndMax();
					pc->setCurrentDisplayedScalarField(sfIndex);
					entity->showSF(true);
					entity->prepareDisplayForRefresh_recursive();
				}
			}
		}
		
		return true;
	}
	
	bool	sfArithmetic(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		Q_ASSERT(!selectedEntities.empty());
		
		ccHObject* entity = selectedEntities[0];
		bool lockedVertices;
		ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(entity,&lockedVertices);
		if (lockedVertices)
		{
			ccUtils::DisplayLockedVerticesWarning(entity->getName(),true);
			return false;
		}
		if (cloud == nullptr)
		{
			return false;
		}
		
		ccScalarFieldArithmeticsDlg sfaDlg(cloud,parent);
		
		if (!sfaDlg.exec())
		{
			return false;
		}
		
		if (!sfaDlg.apply(cloud))
		{
			ccConsole::Error("An error occurred (see Console for more details)");
		}
		
		cloud->showSF(true);
		cloud->prepareDisplayForRefresh_recursive();
		
		return true;
	}
	
	bool	sfFromColor(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		//candidates
		std::unordered_set<ccPointCloud*> clouds;
		
		for (size_t i=0; i<selectedEntities.size(); ++i)
		{
			ccHObject* ent = selectedEntities[i];
			ccPointCloud* cloud = ccHObjectCaster::ToPointCloud(ent);
			if (cloud && ent->hasColors()) //only for clouds (or vertices)
				clouds.insert( cloud );
		}
		
		if (clouds.empty())
			return false;
		
		ccScalarFieldFromColorDlg dialog(parent);
		if (!dialog.exec())
			return false;
		
		const bool exportR = dialog.getRStatus();
		const bool exportG = dialog.getGStatus();
		const bool exportB = dialog.getBStatus();
		const bool exportC = dialog.getCompositeStatus();
		
		for (std::unordered_set<ccPointCloud*>::const_iterator it = clouds.begin(); it != clouds.end(); ++it)
		{
			ccPointCloud* cloud = *it;
			
			std::vector<ccScalarField*> fields(4);
			fields[0] = (exportR ? new ccScalarField(qPrintable(GetFirstAvailableSFName(cloud,"R"))) : 0);
			fields[1] = (exportG ? new ccScalarField(qPrintable(GetFirstAvailableSFName(cloud,"G"))) : 0);
			fields[2] = (exportB ? new ccScalarField(qPrintable(GetFirstAvailableSFName(cloud,"B"))) : 0);
			fields[3] = (exportC ? new ccScalarField(qPrintable(GetFirstAvailableSFName(cloud,"Composite"))) : 0);
			
			//try to instantiate memory for each field
			unsigned count = cloud->size();
			for (size_t i=0; i<fields.size(); ++i)
			{
				if (fields[i] && !fields[i]->reserve(count))
				{
					ccLog::Warning(QString("[sfFromColor] Not enough memory to instantiate SF '%1' on cloud '%2'").arg(fields[i]->getName()).arg(cloud->getName()));
					fields[i]->release();
					fields[i] = nullptr;
				}
			}
			
			//export points
			for (unsigned j=0; j<cloud->size(); ++j)
			{
				const ColorCompType* rgb = cloud->getPointColor(j);
				
				if (fields[0])
					fields[0]->addElement(rgb[0]);
				if (fields[1])
					fields[1]->addElement(rgb[1]);
				if (fields[2])
					fields[2]->addElement(rgb[2]);
				if (fields[3])
					fields[3]->addElement(static_cast<ScalarType>(rgb[0] + rgb[1] + rgb[2])/3);
			}
			
			QString fieldsStr;
			
			for (size_t i=0; i<fields.size(); ++i)
			{
				if (fields[i] == nullptr)
					continue;
				
				fields[i]->computeMinAndMax();
				
				int sfIdx = cloud->getScalarFieldIndexByName(fields[i]->getName());
				if (sfIdx >= 0)
					cloud->deleteScalarField(sfIdx);
				sfIdx = cloud->addScalarField(fields[i]);
				Q_ASSERT(sfIdx >= 0);
				
				if (sfIdx >= 0)
				{
					cloud->setCurrentDisplayedScalarField(sfIdx);
					cloud->showSF(true);
					cloud->prepareDisplayForRefresh();
					
					//mesh vertices?
					if (cloud->getParent() && cloud->getParent()->isKindOf(CC_TYPES::MESH))
					{
						cloud->getParent()->showSF(true);
						cloud->getParent()->prepareDisplayForRefresh();
					}
					
					if (!fieldsStr.isEmpty())
						fieldsStr.append(", ");
					fieldsStr.append(fields[i]->getName());
				}
				else
				{
					ccConsole::Warning(QString("[sfFromColor] Failed to add scalar field '%1' to cloud '%2'?!").arg(fields[i]->getName()).arg(cloud->getName()));
					fields[i]->release();
					fields[i] = nullptr;
				}
			}
			
			if (!fieldsStr.isEmpty())
				ccLog::Print(QString("[sfFromColor] New scalar fields (%1) added to '%2'").arg(fieldsStr).arg(cloud->getName()));
		}

		return true;
	}
	
	bool	processMeshSF(const ccHObject::Container &selectedEntities, ccMesh::MESH_SCALAR_FIELD_PROCESS process, QWidget *parent)
	{
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccHObject* ent = selectedEntities[i];
			if (ent->isKindOf(CC_TYPES::MESH) || ent->isKindOf(CC_TYPES::PRIMITIVE)) //TODO
			{
				ccMesh* mesh = ccHObjectCaster::ToMesh(ent);
				if (mesh == nullptr)
					continue;
				
				ccGenericPointCloud* cloud = mesh->getAssociatedCloud();
				if (cloud == nullptr)
					continue;
				
				if (cloud->isA(CC_TYPES::POINT_CLOUD)) //TODO
				{
					ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
					
					//on active le champ scalaire actuellement affiche
					int sfIdx = pc->getCurrentDisplayedScalarFieldIndex();
					if (sfIdx >= 0)
					{
						pc->setCurrentScalarField(sfIdx);
						mesh->processScalarField(process);
						pc->getCurrentInScalarField()->computeMinAndMax();
						mesh->prepareDisplayForRefresh_recursive();
					}
					else
					{
						ccConsole::Warning(QString("Mesh [%1] vertices have no activated scalar field!").arg(mesh->getName()));
					}
				}
			}
		}
		
		return true;
	}
	
	//////////
	// Normals
	
	bool	computeNormals(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		if (selectedEntities.empty())
		{
			ccConsole::Error("Select at least one point cloud");
			return false;
		}
		
		//look for clouds and meshes
		std::vector<ccPointCloud*> clouds;
		size_t cloudsWithScanGrids = 0;
		std::vector<ccMesh*> meshes;
		PointCoordinateType defaultRadius = 0;
		try
		{
			for (size_t i=0; i<selectedEntities.size(); ++i)
			{
				ccHObject	*entity = selectedEntities[i];
				if (entity->isA(CC_TYPES::POINT_CLOUD))
				{
					ccPointCloud* cloud = static_cast<ccPointCloud*>(entity);
					clouds.push_back(cloud);
					
					if (cloud->gridCount() != 0)
						++cloudsWithScanGrids;
					
					if (defaultRadius == 0)
					{
						//default radius
						defaultRadius = ccNormalVectors::GuessNaiveRadius(cloud);
					}
				}
				else if (entity->isKindOf(CC_TYPES::MESH))
				{
					if (entity->isA(CC_TYPES::MESH))
					{
						ccMesh* mesh = ccHObjectCaster::ToMesh(entity);
						meshes.push_back(mesh);
					}
					else
					{
						ccConsole::Error(QString("Can't compute normals on sub-meshes! Select the parent mesh instead"));
						return false;
					}
				}
			}
		}
		catch (const std::bad_alloc&)
		{
			ccConsole::Error("Not enough memory!");
			return false;
		}
		
		//compute normals for each selected cloud
		if (!clouds.empty())
		{
			ccNormalComputationDlg::SelectionMode selectionMode = ccNormalComputationDlg::WITHOUT_SCAN_GRIDS;
			if (cloudsWithScanGrids)
			{
				if (clouds.size() == cloudsWithScanGrids)
				{
					//all clouds have an associated grid
					selectionMode = ccNormalComputationDlg::WITH_SCAN_GRIDS;
				}
				else
				{
					//only a part of the clouds have an associated grid
					selectionMode = ccNormalComputationDlg::MIXED;
				}
			}
			
			static CC_LOCAL_MODEL_TYPES s_lastModelType = LS;
			static ccNormalVectors::Orientation s_lastNormalOrientation = ccNormalVectors::UNDEFINED;
			static int s_lastMSTNeighborCount = 6;
			static int s_lastKernelSize = 2;
			
			ccNormalComputationDlg ncDlg(selectionMode, parent);
			ncDlg.setLocalModel(s_lastModelType);
			ncDlg.setRadius(defaultRadius);
			ncDlg.setPreferredOrientation(s_lastNormalOrientation);
			ncDlg.setMSTNeighborCount(s_lastMSTNeighborCount);
			ncDlg.setGridKernelSize(s_lastKernelSize);
			if (clouds.size() == 1)
			{
				ncDlg.setCloud(clouds.front());
			}
			
			if (!ncDlg.exec())
				return false;
			
			//normals computation
			CC_LOCAL_MODEL_TYPES model = s_lastModelType = ncDlg.getLocalModel();
			bool useGridStructure = cloudsWithScanGrids && ncDlg.useScanGridsForComputation();
			defaultRadius = ncDlg.getRadius();
			int kernelSize = s_lastKernelSize = ncDlg.getGridKernelSize();
			
			//normals orientation
			bool orientNormals = ncDlg.orientNormals();
			bool orientNormalsWithGrids = cloudsWithScanGrids && ncDlg.useScanGridsForOrientation();
			ccNormalVectors::Orientation preferredOrientation = s_lastNormalOrientation = ncDlg.getPreferredOrientation();
			bool orientNormalsMST = ncDlg.useMSTOrientation();
			int mstNeighbors = s_lastMSTNeighborCount = ncDlg.getMSTNeighborCount();
			
			ccProgressDialog pDlg(true, parent);
			pDlg.setAutoClose(false);

			size_t errors = 0;
			for (size_t i=0; i<clouds.size(); i++)
			{
				ccPointCloud* cloud = clouds[i];
				Q_ASSERT(cloud != nullptr);
				
				bool result = false;
				bool orientNormalsForThisCloud = false;
				if (useGridStructure && cloud->gridCount())
				{
#if 0
					ccPointCloud* newCloud = new ccPointCloud("temp");
					newCloud->reserve(cloud->size());
					for (size_t gi=0; gi<cloud->gridCount(); ++gi)
					{
						const ccPointCloud::Grid::Shared& scanGrid = cloud->grid(gi);
						if (scanGrid && scanGrid->indexes.empty())
						{
							//empty grid, we skip it
							continue;
						}
						ccGLMatrixd toSensor = scanGrid->sensorPosition.inverse();
						
						const int* _indexGrid = &(scanGrid->indexes[0]);
						for (int j=0; j<static_cast<int>(scanGrid->h); ++j)
						{
							for (int i=0; i<static_cast<int>(scanGrid->w); ++i, ++_indexGrid)
							{
								if (*_indexGrid >= 0)
								{
									unsigned pointIndex = static_cast<unsigned>(*_indexGrid);
									const CCVector3* P = cloud->getPoint(pointIndex);
									CCVector3 Q = toSensor * (*P);
									newCloud->addPoint(Q);
								}
							}
						}
						
						addToDB(newCloud);
					}
#endif
					
					//compute normals with the associated scan grid(s)
					orientNormalsForThisCloud = orientNormals && orientNormalsWithGrids;
					result = cloud->computeNormalsWithGrids(model, kernelSize, orientNormalsForThisCloud, &pDlg);
				}
				else
				{
					//compute normals with the octree
					orientNormalsForThisCloud = orientNormals && (preferredOrientation != ccNormalVectors::UNDEFINED);
					result = cloud->computeNormalsWithOctree(model, orientNormals ? preferredOrientation : ccNormalVectors::UNDEFINED, defaultRadius, &pDlg);
				}
				
				//do we need to orient the normals? (this may have been already done if 'orientNormalsForThisCloud' is true)
				if (result && orientNormals && !orientNormalsForThisCloud)
				{
					if (cloud->gridCount() && orientNormalsWithGrids)
					{
						//we can still use the grid structure(s) to orient the normals!
						result = cloud->orientNormalsWithGrids();
					}
					else if (orientNormalsMST)
					{
						//use Minimum Spanning Tree to resolve normals direction
						result = cloud->orientNormalsWithMST(mstNeighbors, &pDlg);
					}
				}
				
				if (!result)
				{
					++errors;
				}
				
				cloud->prepareDisplayForRefresh();
			}
			
			if (errors != 0)
			{
				if (errors < clouds.size())
					ccConsole::Error("Failed to compute or orient the normals on some clouds! (see console)");
				else
					ccConsole::Error("Failed to compute or orient the normals! (see console)");
			}
		}
		
		//compute normals for each selected mesh
		if (!meshes.empty())
		{
			QMessageBox question( QMessageBox::Question,
										 "Mesh normals",
										 "Compute per-vertex normals (smooth) or per-triangle (faceted)?",
										 QMessageBox::NoButton,
										 parent);
			
			QPushButton* perVertexButton   = question.addButton("Per-vertex", QMessageBox::YesRole);
			QPushButton* perTriangleButton = question.addButton("Per-triangle", QMessageBox::NoRole);
			
			question.exec();
			
			bool computePerVertexNormals = (question.clickedButton() == perVertexButton);
			
			for (size_t i=0; i<meshes.size(); i++)
			{
				ccMesh* mesh = meshes[i];
				Q_ASSERT(mesh != nullptr);
				
				//we remove temporarily the mesh as its normals may be removed (and they can be a child object)
				MainWindow* instance = dynamic_cast<MainWindow*>(parent);
				MainWindow::ccHObjectContext objContext;
				if (instance)
					objContext = instance->removeObjectTemporarilyFromDBTree(mesh);
				mesh->clearTriNormals();
				mesh->showNormals(false);
				bool result = mesh->computeNormals(computePerVertexNormals);
				if (instance)
					instance->putObjectBackIntoDBTree(mesh,objContext);
				
				if (!result)
				{
					ccConsole::Error(QString("Failed to compute normals on mesh '%1'").arg(mesh->getName()));
					continue;
				}
				mesh->prepareDisplayForRefresh_recursive();
			}
		}
		
		return true;
	}
	
	bool	invertNormals(const ccHObject::Container &selectedEntities)
	{
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccHObject* ent = selectedEntities[i];
			bool lockedVertices;
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
			if (lockedVertices)
			{
				ccUtils::DisplayLockedVerticesWarning(ent->getName(),selNum == 1);
				continue;
			}
			
			if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) // TODO
			{
				ccPointCloud* ccCloud = static_cast<ccPointCloud*>(cloud);
				if (ccCloud->hasNormals())
				{
					ccCloud->invertNormals();
					ccCloud->showNormals(true);
					ccCloud->prepareDisplayForRefresh_recursive();
				}
			}
		}
		
		return true;
	}
	
	bool	orientNormalsFM(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		if (selectedEntities.empty())
		{
			ccConsole::Error("Select at least one point cloud");
			return false;
		}
		
		bool ok = false;
		const int s_defaultLevel = 6;
		int value = QInputDialog::getInt(parent,
													"Orient normals (FM)", "Octree level",
													s_defaultLevel,
													1, CCLib::DgmOctree::MAX_OCTREE_LEVEL,
													1,
													&ok);
		if (!ok)
			return false;
		
		Q_ASSERT(value >= 0 && value <= 255);
		
		unsigned char level = static_cast<unsigned char>(value);
		
		ccProgressDialog pDlg(false, parent);
		pDlg.setAutoClose(false);

		size_t errors = 0;
		for (size_t i=0; i<selectedEntities.size(); i++)
		{
			ccHObject* entity = selectedEntities[i];
			
			if (!entity->isA(CC_TYPES::POINT_CLOUD))
				continue;
			
			ccPointCloud* cloud = static_cast<ccPointCloud*>(entity);
			
			if (!cloud->hasNormals())
			{
				ccConsole::Warning(QString("Cloud '%1' has no normals!").arg(cloud->getName()));
				continue;
			}
			
			//orient normals with Fast Marching
			if (cloud->orientNormalsWithFM(level, &pDlg))
			{
				cloud->prepareDisplayForRefresh();
			}
			else
			{
				++errors;
			}
		}
		
		if (errors)
		{
			ccConsole::Error(QString("Process failed (check console)"));
		}
		else
		{
			ccLog::Warning("Normals have been oriented: you may still have to globally invert the cloud normals however (Edit > Normals > Invert).");
		}
		
		return true;
	}
	
	bool	orientNormalsMST(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		if (selectedEntities.empty())
		{
			ccConsole::Error("Select at least one point cloud");
			return false;
		}
		
		bool ok = false;
		static unsigned s_defaultKNN = 6;
		unsigned kNN = static_cast<unsigned>(QInputDialog::getInt(parent,
																					 "Neighborhood size", "Neighbors",
																					 s_defaultKNN ,
																					 1, 1000,
																					 1,
																					 &ok));
		if (!ok)
			return false;
		
		s_defaultKNN = kNN;
		
		ccProgressDialog pDlg(true, parent);
		pDlg.setAutoClose(false);
		
		size_t errors = 0;
		for (size_t i=0; i<selectedEntities.size(); i++)
		{
			ccHObject* entity = selectedEntities[i];
			
			if (!entity->isA(CC_TYPES::POINT_CLOUD))
				continue;
			
			ccPointCloud* cloud = static_cast<ccPointCloud*>(entity);
			
			if (!cloud->hasNormals())
			{
				ccConsole::Warning(QString("Cloud '%1' has no normals!").arg(cloud->getName()));
				continue;
			}
			
			//use Minimum Spanning Tree to resolve normals direction
			if (cloud->orientNormalsWithMST(kNN, &pDlg))
			{
				cloud->prepareDisplayForRefresh();
			}
			else
			{
				ccConsole::Warning(QString("Process failed on cloud '%1'").arg(cloud->getName()));
				++errors;
			}
		}
		
		if (errors)
		{
			ccConsole::Error(QString("Process failed (check console)"));
		}
		else
		{
			ccLog::Warning("Normals have been oriented: you may still have to globally invert the cloud normals however (Edit > Normals > Invert).");
		}
		
		return true;
	}
	
	bool	convertNormalsTo(const ccHObject::Container &selectedEntities, NORMAL_CONVERSION_DEST dest)
	{
		unsigned errorCount = 0;
		
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccHObject* ent = selectedEntities[i];
			bool lockedVertices = false;
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
			if (lockedVertices)
			{
				ccUtils::DisplayLockedVerticesWarning(ent->getName(),selNum == 1);
				continue;
			}
			
			if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) // TODO
			{
				ccPointCloud* ccCloud = static_cast<ccPointCloud*>(cloud);
				if (ccCloud->hasNormals())
				{
					bool success = true;
					switch(dest)
					{
						case NORMAL_CONVERSION_DEST::HSV_COLORS:
						{
							success = ccCloud->convertNormalToRGB();
							if (success)
							{
								ccCloud->showSF(false);
								ccCloud->showNormals(false);
								ccCloud->showColors(true);
							}
						}
							break;
						case NORMAL_CONVERSION_DEST::DIP_DIR_SFS:
						{
							//get/create 'dip' scalar field
							int dipSFIndex = ccCloud->getScalarFieldIndexByName(CC_DEFAULT_DIP_SF_NAME);
							if (dipSFIndex < 0)
								dipSFIndex = ccCloud->addScalarField(CC_DEFAULT_DIP_SF_NAME);
							if (dipSFIndex < 0)
							{
								ccLog::Warning("[ccEntityAction::convertNormalsTo] Not enough memory!");
								success = false;
								break;
							}
							
							//get/create 'dip direction' scalar field
							int dipDirSFIndex = ccCloud->getScalarFieldIndexByName(CC_DEFAULT_DIP_DIR_SF_NAME);
							if (dipDirSFIndex < 0)
								dipDirSFIndex = ccCloud->addScalarField(CC_DEFAULT_DIP_DIR_SF_NAME);
							if (dipDirSFIndex < 0)
							{
								ccCloud->deleteScalarField(dipSFIndex);
								ccLog::Warning("[ccEntityAction::convertNormalsTo] Not enough memory!");
								success = false;
								break;
							}
							
							ccScalarField* dipSF = static_cast<ccScalarField*>(ccCloud->getScalarField(dipSFIndex));
							ccScalarField* dipDirSF = static_cast<ccScalarField*>(ccCloud->getScalarField(dipDirSFIndex));
							Q_ASSERT(dipSF && dipDirSF);
							
							success = ccCloud->convertNormalToDipDirSFs(dipSF, dipDirSF);
							
							if (success)
							{
								//apply default 360 degrees color scale!
								ccColorScale::Shared scale = ccColorScalesManager::GetDefaultScale(ccColorScalesManager::HSV_360_DEG);
								dipSF->setColorScale(scale);
								dipDirSF->setColorScale(scale);
								ccCloud->setCurrentDisplayedScalarField(dipDirSFIndex); //dip dir. seems more interesting by default
								ccCloud->showSF(true);
							}
							else
							{
								ccCloud->deleteScalarField(dipSFIndex);
								ccCloud->deleteScalarField(dipDirSFIndex);
							}
						}
							break;
						default:
							Q_ASSERT(false);
							ccLog::Warning("[ccEntityAction::convertNormalsTo] Internal error: unhandled destination!");
							success = false;
							i = selNum; //no need to process the selected entities anymore!
							break;
					}
					
					if (success)
					{
						ccCloud->prepareDisplayForRefresh_recursive();
					}
					else
					{
						++errorCount;
					}
				}
			}
		}
		
		//errors should have been sent to console as warnings
		if (errorCount)
		{
			ccConsole::Error("Error(s) occurred! (see console)");
		}
		
		return true;
	}
	
	//////////
	// Octree

	bool computeOctree(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		ccBBox bbox;
		std::unordered_set<ccGenericPointCloud*> clouds;
		size_t selNum = selectedEntities.size();
		PointCoordinateType maxBoxSize = -1;
		for (size_t i = 0; i < selNum; ++i)
		{
			ccHObject* ent = selectedEntities[i];

			//specific test for locked vertices
			bool lockedVertices = false;
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent, &lockedVertices);
			if (cloud && lockedVertices)
			{
				ccUtils::DisplayLockedVerticesWarning(ent->getName(), selNum == 1);
				continue;
			}
			clouds.insert(cloud);

			//we look for the biggest box so as to define the "minimum cell size"
			const ccBBox thisBBox = cloud->getOwnBB();
			if (thisBBox.isValid())
			{
				CCVector3 dd = thisBBox.maxCorner() - thisBBox.minCorner();
				PointCoordinateType maxd = std::max(dd.x, std::max(dd.y, dd.z));
				if (maxBoxSize < 0.0 || maxd > maxBoxSize)
					maxBoxSize = maxd;
			}
			bbox += thisBBox;
		}

		if (clouds.empty() || maxBoxSize < 0.0)
		{
			ccLog::Warning("[doActionComputeOctree] No eligible entities in selection!");
			return false;
		}

		//min(cellSize) = max(dim)/2^N with N = max subidivision level
		const double minCellSize = static_cast<double>(maxBoxSize) / (1 << ccOctree::MAX_OCTREE_LEVEL);

		ccComputeOctreeDlg coDlg(bbox, minCellSize, parent);
		if (!coDlg.exec())
			return false;

		ccProgressDialog pDlg(true, parent);
		pDlg.setAutoClose(false);

		//if we must use a custom bounding box, we update 'bbox'
		if (coDlg.getMode() == ccComputeOctreeDlg::CUSTOM_BBOX)
			bbox = coDlg.getCustomBBox();

		for (std::unordered_set<ccGenericPointCloud*>::iterator it = clouds.begin(); it != clouds.end(); ++it)
		{
			ccGenericPointCloud* cloud = *it;

			//we temporarily detach entity, as it may undergo
			//"severe" modifications (octree deletion, etc.) --> see ccPointCloud::computeOctree
			MainWindow* instance = dynamic_cast<MainWindow*>(parent);
			MainWindow::ccHObjectContext objContext;
			if (instance)
				objContext = instance->removeObjectTemporarilyFromDBTree(cloud);

			//computation
			QElapsedTimer eTimer;
			eTimer.start();
			ccOctree::Shared octree(0);
			switch (coDlg.getMode())
			{
			case ccComputeOctreeDlg::DEFAULT:
				octree = cloud->computeOctree(&pDlg);
				break;
			case ccComputeOctreeDlg::MIN_CELL_SIZE:
			case ccComputeOctreeDlg::CUSTOM_BBOX:
			{
				//for a cell-size based custom box, we must update it for each cloud!
				if (coDlg.getMode() == ccComputeOctreeDlg::MIN_CELL_SIZE)
				{
					double cellSize = coDlg.getMinCellSize();
					PointCoordinateType halfBoxWidth = static_cast<PointCoordinateType>(cellSize * (1 << ccOctree::MAX_OCTREE_LEVEL) / 2.0);
					CCVector3 C = cloud->getOwnBB().getCenter();
					bbox = ccBBox(	C - CCVector3(halfBoxWidth, halfBoxWidth, halfBoxWidth),
									C + CCVector3(halfBoxWidth, halfBoxWidth, halfBoxWidth));
				}
				cloud->deleteOctree();
				octree = ccOctree::Shared(new ccOctree(cloud));
				if (octree->build(bbox.minCorner(), bbox.maxCorner(), 0, 0, &pDlg) > 0)
				{
					ccOctreeProxy* proxy = new ccOctreeProxy(octree);
					proxy->setDisplay(cloud->getDisplay());
					cloud->addChild(proxy);
				}
				else
				{
					octree.clear();
				}
			}
			break;
			default:
				Q_ASSERT(false);
				return false;
			}
			qint64 elapsedTime_ms = eTimer.elapsed();

			//put object back in tree
			if (instance)
				instance->putObjectBackIntoDBTree(cloud, objContext);

			if (octree)
			{
				ccConsole::Print("[doActionComputeOctree] Timing: %2.3f s", static_cast<double>(elapsedTime_ms) / 1000.0);
				cloud->setEnabled(true); //for mesh vertices!
				ccOctreeProxy* proxy = cloud->getOctreeProxy();
				assert(proxy);
				proxy->setVisible(true);
				proxy->prepareDisplayForRefresh();
			}
			else
			{
				ccConsole::Warning(QString("Octree computation on cloud '%1' failed!").arg(cloud->getName()));
			}
		}

		return true;
	}
	
	//////////
	// Properties
	
	bool	clearProperty(ccHObject::Container selectedEntities, CLEAR_PROPERTY property, QWidget *parent)
	{	
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccHObject* ent = selectedEntities[i];
			
			//specific case: clear normals on a mesh
			if (property == CLEAR_PROPERTY::NORMALS && ( ent->isA(CC_TYPES::MESH) /*|| ent->isKindOf(CC_TYPES::PRIMITIVE)*/ )) //TODO
			{
				ccMesh* mesh = ccHObjectCaster::ToMesh(ent);
				if (mesh->hasTriNormals())
				{
					mesh->showNormals(false);
					
					MainWindow* instance = dynamic_cast<MainWindow*>(parent);
					MainWindow::ccHObjectContext objContext;
					if (instance)
						objContext = instance->removeObjectTemporarilyFromDBTree(mesh);
					mesh->clearTriNormals();
					if (instance)
						instance->putObjectBackIntoDBTree(mesh,objContext);
					
					ent->prepareDisplayForRefresh();
					continue;
				}
				else if (mesh->hasNormals()) //per-vertex normals?
				{
					if (mesh->getParent()
						 && (mesh->getParent()->isA(CC_TYPES::MESH)/*|| mesh->getParent()->isKindOf(CC_TYPES::PRIMITIVE)*/) //TODO
						 && ccHObjectCaster::ToMesh(mesh->getParent())->getAssociatedCloud() == mesh->getAssociatedCloud())
					{
						ccLog::Warning("[doActionClearNormals] Can't remove per-vertex normals on a sub mesh!");
					}
					else //mesh is alone, we can freely remove normals
					{
						if (mesh->getAssociatedCloud() && mesh->getAssociatedCloud()->isA(CC_TYPES::POINT_CLOUD))
						{
							mesh->showNormals(false);
							static_cast<ccPointCloud*>(mesh->getAssociatedCloud())->unallocateNorms();
							mesh->prepareDisplayForRefresh();
							continue;
						}
					}
				}
			}
			
			bool lockedVertices;
			ccGenericPointCloud* cloud = ccHObjectCaster::ToGenericPointCloud(ent,&lockedVertices);
			if (lockedVertices)
			{
				ccUtils::DisplayLockedVerticesWarning(ent->getName(),selNum == 1);
				continue;
			}
			
			if (cloud && cloud->isA(CC_TYPES::POINT_CLOUD)) // TODO
			{
				switch (property)
				{
					case CLEAR_PROPERTY::COLORS: //colors
						if (cloud->hasColors())
						{
							static_cast<ccPointCloud*>(cloud)->unallocateColors();
							ent->prepareDisplayForRefresh();
						}
						break;
					case CLEAR_PROPERTY::NORMALS: //normals
						if (cloud->hasNormals())
						{
							static_cast<ccPointCloud*>(cloud)->unallocateNorms();
							ent->prepareDisplayForRefresh();
						}
						break;
					case CLEAR_PROPERTY::CURRENT_SCALAR_FIELD: //current sf
						if (cloud->hasDisplayedScalarField())
						{
							ccPointCloud* pc = static_cast<ccPointCloud*>(cloud);
							pc->deleteScalarField(pc->getCurrentDisplayedScalarFieldIndex());
							ent->prepareDisplayForRefresh();
						}
						break;
					case CLEAR_PROPERTY::ALL_SCALAR_FIELDS: //all sf
						if (cloud->hasScalarFields())
						{
							static_cast<ccPointCloud*>(cloud)->deleteAllScalarFields();
							ent->prepareDisplayForRefresh();
						}
						break;
				}
			}
		}
		
		return true;
	}
	
	bool	toggleProperty(const ccHObject::Container &selectedEntities, TOGGLE_PROPERTY property)
	{
		ccHObject baseEntities;
		ConvertToGroup(selectedEntities, baseEntities, ccHObject::DP_NONE);
		
		for (unsigned i=0; i<baseEntities.getChildrenNumber(); ++i)
		{
			ccHObject* child = baseEntities.getChild(i);
			switch(property)
			{
				case TOGGLE_PROPERTY::ACTIVE:
					child->toggleActivation/*_recursive*/();
					break;
				case TOGGLE_PROPERTY::VISIBLE:
					child->toggleVisibility_recursive();
					break;
				case TOGGLE_PROPERTY::COLOR:
					child->toggleColors_recursive();
					break;
				case TOGGLE_PROPERTY::NORMALS:
					child->toggleNormals_recursive();
					break;
				case TOGGLE_PROPERTY::SCALAR_FIELD:
					child->toggleSF_recursive();
					break;
				case TOGGLE_PROPERTY::MATERIAL:
					child->toggleMaterials_recursive();
					break;
				case TOGGLE_PROPERTY::NAME:
					child->toggleShowName_recursive();
					break;
				default:
					Q_ASSERT(false);
					return false;
			}
			child->prepareDisplayForRefresh_recursive();
		}
		
		return true;
	}
	
	//////////
	// Stats
	
	bool	statisticalTest(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		ccPickOneElementDlg poeDlg("Distribution","Choose distribution",parent);
		poeDlg.addElement("Gauss");
		poeDlg.addElement("Weibull");
		poeDlg.setDefaultIndex(0);
		if (!poeDlg.exec())
		{
			return false;
		}
		
		int distribIndex = poeDlg.getSelectedIndex();
		
		ccStatisticalTestDlg* sDlg = nullptr;
		switch (distribIndex)
		{
			case 0: //Gauss
				sDlg = new ccStatisticalTestDlg("mu","sigma",QString(),"Local Statistical Test (Gauss)",parent);
				break;
			case 1: //Weibull
				sDlg = new ccStatisticalTestDlg("a","b","shift","Local Statistical Test (Weibull)",parent);
				break;
			default:
				ccConsole::Error("Invalid distribution!");
				return false;
		}
		
		if (!sDlg->exec())
		{
			sDlg->deleteLater();
			return false;
		}
		
		//build up corresponding distribution
		CCLib::GenericDistribution* distrib = nullptr;
		{
			ScalarType a = static_cast<ScalarType>(sDlg->getParam1());
			ScalarType b = static_cast<ScalarType>(sDlg->getParam2());
			ScalarType c = static_cast<ScalarType>(sDlg->getParam3());
			
			switch (distribIndex)
			{
				case 0: //Gauss
				{
					CCLib::NormalDistribution* N = new CCLib::NormalDistribution();
					N->setParameters(a,b*b); //warning: we input sigma2 here (not sigma)
					distrib = static_cast<CCLib::GenericDistribution*>(N);
					break;
				}
				case 1: //Weibull
					CCLib::WeibullDistribution* W = new CCLib::WeibullDistribution();
					W->setParameters(a,b,c);
					distrib = static_cast<CCLib::GenericDistribution*>(W);
					break;
			}
		}
		
		const double pChi2 = sDlg->getProba();
		const int nn = sDlg->getNeighborsNumber();
		
		ccProgressDialog pDlg(true, parent);
		pDlg.setAutoClose(false);
		
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccPointCloud* pc = ccHObjectCaster::ToPointCloud(selectedEntities[i]);
			if (pc == nullptr)
			{
				// TODO handle error?
				continue;
			}
			
			//we apply method on currently displayed SF
			ccScalarField* inSF = pc->getCurrentDisplayedScalarField();
			if (inSF == nullptr)
			{
				// TODO handle error?
				continue;
			}
			
			Q_ASSERT(inSF->isAllocated());
			
			//force SF as 'OUT' field (in case of)
			const int outSfIdx = pc->getCurrentDisplayedScalarFieldIndex();
			pc->setCurrentOutScalarField(outSfIdx);
			
			//force Chi2 Distances field as 'IN' field (create it by the way if necessary)
			int chi2SfIdx = pc->getScalarFieldIndexByName(CC_CHI2_DISTANCES_DEFAULT_SF_NAME);
			
			if (chi2SfIdx < 0)
				chi2SfIdx = pc->addScalarField(CC_CHI2_DISTANCES_DEFAULT_SF_NAME);
			
			if (chi2SfIdx < 0)
			{
				ccConsole::Error("Couldn't allocate a new scalar field for computing chi2 distances! Try to free some memory ...");
				break;
			}
			pc->setCurrentInScalarField(chi2SfIdx);
			
			//compute octree if necessary
			ccOctree::Shared theOctree = pc->getOctree();
			if (!theOctree)
			{
				theOctree = pc->computeOctree(&pDlg);
				if (!theOctree)
				{
					ccConsole::Error(QString("Couldn't compute octree for cloud '%1'!").arg(pc->getName()));
					break;
				}
			}
			
			QElapsedTimer eTimer;
			eTimer.start();
			
			double chi2dist = CCLib::StatisticalTestingTools::testCloudWithStatisticalModel(distrib, pc, nn, pChi2, &pDlg, theOctree.data());
			
			ccConsole::Print("[Chi2 Test] Timing: %3.2f ms.", eTimer.elapsed() / 1000.0);
			ccConsole::Print("[Chi2 Test] %s test result = %f", distrib->getName(), chi2dist);
			
			//we set the theoretical Chi2 distance limit as the minimum displayed SF value so that all points below are grayed
			{
				ccScalarField* chi2SF = static_cast<ccScalarField*>(pc->getCurrentInScalarField());
				Q_ASSERT(chi2SF);
				chi2SF->computeMinAndMax();
				chi2dist *= chi2dist;
				chi2SF->setMinDisplayed(static_cast<ScalarType>(chi2dist));
				chi2SF->setSymmetricalScale(false);
				chi2SF->setSaturationStart(static_cast<ScalarType>(chi2dist));
				//chi2SF->setSaturationStop(chi2dist);
				
				pc->setCurrentDisplayedScalarField(chi2SfIdx);
				pc->showSF(true);
				pc->prepareDisplayForRefresh_recursive();
			}
		}
		
		delete distrib;
		distrib = nullptr;
		
		sDlg->deleteLater();
		
		return true;
	}
	
	bool	computeStatParams(const ccHObject::Container &selectedEntities, QWidget *parent)
	{
		ccPickOneElementDlg pDlg("Distribution","Distribution Fitting",parent);
		pDlg.addElement("Gauss");
		pDlg.addElement("Weibull");
		pDlg.setDefaultIndex(0);
		if (!pDlg.exec())
			return false;
		
		CCLib::GenericDistribution* distrib = nullptr;
		{
			switch (pDlg.getSelectedIndex())
			{
				case 0: //GAUSS
					distrib = new CCLib::NormalDistribution();
					break;
				case 1: //WEIBULL
					distrib = new CCLib::WeibullDistribution();
					break;
				default:
					Q_ASSERT(false);
					return false;
			}
		}
		Q_ASSERT(distrib != nullptr);
		
		size_t selNum = selectedEntities.size();
		for (size_t i=0; i<selNum; ++i)
		{
			ccPointCloud* pc = ccHObjectCaster::ToPointCloud(selectedEntities[i]);
			if (pc == nullptr)
			{
				// TODO report error?
				continue;
			}
			
			//we apply method on currently displayed SF
			ccScalarField* sf = pc->getCurrentDisplayedScalarField();
			if (sf == nullptr)
			{
				// TODO report error?
				continue;
			}
			
			Q_ASSERT(sf->isAllocated());
			
			//force SF as 'OUT' field (in case of)
			const int outSfIdx = pc->getCurrentDisplayedScalarFieldIndex();
			Q_ASSERT(outSfIdx >= 0);
			pc->setCurrentOutScalarField(outSfIdx);
			
			if (distrib->computeParameters(pc))
			{
				QString description;
				
				const unsigned precision = ccGui::Parameters().displayedNumPrecision;
				switch (pDlg.getSelectedIndex())
				{
					case 0: //GAUSS
					{
						CCLib::NormalDistribution* normal = static_cast<CCLib::NormalDistribution*>(distrib);
						description = QString("mean = %1 / std.dev. = %2").arg(normal->getMu(),0,'f',precision).arg(sqrt(normal->getSigma2()),0,'f',precision);
					}
						break;
					case 1: //WEIBULL
					{
						CCLib::WeibullDistribution* weibull = static_cast<CCLib::WeibullDistribution*>(distrib);
						ScalarType a,b;
						weibull->getParameters(a,b);
						description = QString("a = %1 / b = %2 / shift = %3").arg(a,0,'f',precision).arg(b,0,'f',precision).arg(weibull->getValueShift(),0,'f',precision);
					}
						break;
					default:
						Q_ASSERT(false);
						return false;
				}
				description.prepend(QString("%1: ").arg(distrib->getName()));
				ccConsole::Print(QString("[Distribution fitting] %1").arg(description));
				
				//Auto Chi2
				const unsigned numberOfClasses = static_cast<unsigned>(ceil(sqrt(static_cast<double>(pc->size()))));
				std::vector<unsigned> histo;
				std::vector<double> npis;
				try
				{
					histo.resize(numberOfClasses,0);
					npis.resize(numberOfClasses,0.0);
				}
				catch (const std::bad_alloc&)
				{
					ccConsole::Warning("[Distribution fitting] Not enough memory!");
					continue;
				}
				
				unsigned finalNumberOfClasses = 0;
				const double chi2dist = CCLib::StatisticalTestingTools::computeAdaptativeChi2Dist(distrib,pc,0,finalNumberOfClasses,false,0,0,&(histo[0]),&(npis[0]));
				
				if (chi2dist >= 0.0)
				{
					ccConsole::Print("[Distribution fitting] %s: Chi2 Distance = %f",distrib->getName(),chi2dist);
				}
				else
				{
					ccConsole::Warning("[Distribution fitting] Failed to compute Chi2 distance?!");
					continue;
				}
				
				//show histogram
				ccHistogramWindowDlg* hDlg = new ccHistogramWindowDlg(parent);
				hDlg->setWindowTitle("[Distribution fitting]");
				
				ccHistogramWindow* histogram = hDlg->window();
				histogram->fromBinArray(histo,sf->getMin(),sf->getMax());
				histo.clear();
				histogram->setCurveValues(npis);
				npis.clear();
				histogram->setTitle(description);
				histogram->setColorScheme(ccHistogramWindow::USE_CUSTOM_COLOR_SCALE);
				histogram->setColorScale(sf->getColorScale());
				histogram->setAxisLabels(sf->getName(),"Count");
				histogram->refresh();
				
				hDlg->show();
			}
			else
			{
				ccConsole::Warning(QString("[Entity: %1]-[SF: %2] Couldn't compute distribution parameters!").arg(pc->getName()).arg(pc->getScalarFieldName(outSfIdx)));
			}
		}
		
		delete distrib;
		distrib = nullptr;
		
		return true;
	}
}
