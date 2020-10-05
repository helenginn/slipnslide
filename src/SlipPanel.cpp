// Slip n Slide
// Copyright (C) 2019 Helen Ginn
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
// 
// Please email: vagabond @ hginn.co.uk for more details.

#include <iostream>
#include <float.h>
#include <FileReader.h>
#include <iomanip>
#include "vec_utils.h"
#include "SlipPanel.h"
#include "SlipObject.h"
#include "Curve.h"
#include "CurveView.h"
#include "shaders/vari_z.h"
#include <crystfel/reflist.h>
#include <crystfel/geometry.h>

#define DESELECTED_COLOUR (0.5)
#define SELECTED_COLOUR (1.0)

SlipPanel::SlipPanel(struct panel *p) : SlipObject()
{
	_single = true;
	_panel = p;
	_isSelected = false;
	updateTmpPanelValues();
	createVertices();

	_vString = variable_z_vsh();
	_fString = variable_z_fsh();
}

SlipPanel::SlipPanel()
{
	_single = false;
	_isSelected = false;
	_panel = NULL;
}

void SlipPanel::addPanel(SlipPanel *other)
{
	if (_single)
	{
		return;
	}

	if (isSelected())
	{
		other->recolour(SELECTED_COLOUR, 0, 0);
	}
	
	_subpanels.push_back(other);
}

void SlipPanel::updateTmpPanelValues()
{
	if (_single == false)
	{
		return;
	}

	double mm = _panel->res / 1000;
	vec3 fs = make_vec3(_panel->fsx, _panel->fsy, _panel->fsz);
	vec3 ss = make_vec3(_panel->ssx, _panel->ssy, _panel->ssz);
	double d = mm * 1000 *  (_panel->clen + _panel->coffset);
	vec3 corner = make_vec3(_panel->cnx, _panel->cny, d);

	_corner = corner;
	vec3_mult(&_corner, 1 / mm);
	_fs = fs;
	_ss = ss;
	_width = _panel->w / mm;
	_height = _panel->h / mm;
	_px_per_m = mm;
}

void SlipPanel::createVertices()
{
	if (_single == false)
	{
		return;
	}

	_vertices.clear();
	_indices.clear();
	
	_indices.push_back(0);
	_indices.push_back(1);
	_indices.push_back(2);
	_indices.push_back(2);
	_indices.push_back(1);
	_indices.push_back(3);
	
	Vertex v;
	memset(v.pos, 0, sizeof(Vertex));

	v.color[0] = 0.5;
	v.color[3] = 1;
	_vertices.push_back(v);
	_vertices.push_back(v);
	_vertices.push_back(v);
	_vertices.push_back(v);

	updateVertices();
}

void SlipPanel::updateVertices()
{
	if (_single == false)
	{
		return;
	}

	pos_from_vec(_vertices[0].pos, _corner);
	vec3 fcorn = _fs;
	vec3_mult(&fcorn, _width);
	vec3_add_to_vec3(&fcorn, _corner);
	pos_from_vec(_vertices[1].pos, fcorn);

	vec3 scorn = _ss;
	vec3_mult(&scorn, _height);
	vec3_add_to_vec3(&scorn, _corner);
	pos_from_vec(_vertices[2].pos, scorn);

	fcorn = _fs;
	vec3_mult(&fcorn, _width);
	vec3_add_to_vec3(&scorn, fcorn);
	pos_from_vec(_vertices[3].pos, scorn);
}

bool SlipPanel::isValidPanelMember(struct panel *p)
{
	if (_single)
	{
		return (p == panelPtr());
	}

	for (size_t i = 0; i < _subpanels.size(); i++)
	{
		if (_subpanels[i]->isValidPanelMember(p))
		{
			return true;
		}
	}
	
	return false;
}

void SlipPanel::getPeaksFromImage(struct image *im)
{
	ImageFeatureList *list = im->features;
	size_t start = _peaks.size();

	for (int i = 0; i < image_feature_count(list); i++)
	{
		struct imagefeature *peak;
		peak = image_get_feature(list, i);
		peak->parent = im;

		if (isValidPanelMember(peak->p))
		{
			_peaks.push_back(*peak);
		}
	}

	size_t end = _peaks.size();
	
	if (end != start)
	{
		_images.push_back(im);

		if (start == 0)
		{
			_imageStarts.push_back(start);
		}
		
		_imageStarts.push_back(end);
	}
}

void SlipPanel::updatePeaks()
{
	for (size_t i = 0; i < _peaks.size(); i++)
	{
		struct imagefeature *peak = &_peaks[i];
		struct image *im = peak->parent;

		double k = 1e-10 / im->lambda; /* inverse Angs */
		double fs = peak->fs;
		double ss = peak->ss;
		panel *p = peak->p;

		/* Calculate 3D position of given position, in m */ 
		double x = (p->cnx  + fs*p->fsx + ss*p->ssx);
		x /= p->res;
		double y = (p->cny  + fs*p->fsy + ss*p->ssy);
		y /= p->res;
		double z = (fs*p->fsz + ss*p->ssz);
		z /= p->res;
		z += (p->clen + p->coffset);
		
		vec3 v = make_vec3(x, y, z);

		vec3_set_length(&v, k);

		peak->rx = v.x;
		peak->ry = v.y;
		peak->rz = v.z;
	}
}

struct imagefeature *SlipPanel::findClosestPeak(struct image *im,
                                                double fs, double ss)
{
	double closest = FLT_MAX;
	struct imagefeature *peak = NULL;
	const double max = 20;

	for (size_t i = 0; i < _peaks.size(); i++)
	{
		if (_peaks[i].parent != im)
		{
			continue;
		}

		double dfs = (_peaks[i].fs - fs);
		
		if (dfs > max || dfs < -max)
		{
			continue;
		}

		double dss = (_peaks[i].ss - ss);
		
		if (dss > max || dss < -max)
		{
			continue;
		}
		
		double dist = dfs * dfs + dss * dss;
		
		if (dist < closest)
		{
			closest = dist;
			peak = &_peaks[i];
		}
	}

	return peak;
}

vec3 SlipPanel::rayTraceToPanel(struct panel *p, vec3 dir)
{
	vec3 corner = make_vec3(p->cnx / p->res, p->cny / p->res,
	                        p->clen + p->coffset);
	vec3 diff1 = make_vec3(p->fsx, p->fsy, p->fsz);
	vec3 diff2 = make_vec3(p->ssx, p->ssy, p->ssz);

	vec3 cross = vec3_cross_vec3(diff1, diff2);
	vec3_set_length(&cross, 1);

	double denom = vec3_dot_vec3(dir, cross);
	double nom = vec3_dot_vec3(corner, cross);  
	double d = nom / denom;

	vec3_mult(&dir, d);

	return dir;   
}  

void SlipPanel::updateTarget(Curve *c, bool refresh)
{
	if (refresh)
	{
		updatePeaks();
	}
	
	double asx, asy, asz;
	double bsx, bsy, bsz;
	double csx, csy, csz;

	if (_pairs.size() == 0)
	{
		for (size_t i = 0; i < _images.size() && i < 10; i++)
		{
			struct image *im = _images[i];

			for (int j = 0; j < im->n_crystals; j++)
			{
				Crystal *cryst = im->crystals[j];
				RefListIterator *it;
				RefList *refs = crystal_get_reflections(cryst);
				Reflection *ref = first_refl(refs, &it);
				UnitCell *cell = crystal_get_cell(cryst);

				cell_get_reciprocal(cell, &asx, &asy, &asz,
				                    &bsx, &bsy, &bsz,
				                    &csx, &csy, &csz);

				if (ref == NULL)
				{
					continue;
				}
				
				if (!isValidPanelMember(get_panel(ref)))
				{
					continue;
				}

				while (true)
				{
					ref = (next_refl(ref, it));
					if (ref == NULL)
					{
						break;
					}

					double fs, ss;
					get_detector_pos(ref, &fs, &ss);

					struct imagefeature *peak = findClosestPeak(im, fs, ss);

					if (peak == NULL)
					{
						continue;
					}
					
					set_temp1(ref, peak->fs);
					set_temp2(ref, peak->ss);

					RefPeak rp;
					rp.ref = ref;
					rp.peak = peak;
					_pairs.push_back(rp);
				}
			}
		}
	}
	
	c->clear();
	c->setPointData(true);
	
	for (size_t i = 0; i < _images.size() && i < 10; i++)
	{
		struct image *im = _images[i];

		for (int j = 0; j < im->n_crystals; j++)
		{
			Crystal *cryst = im->crystals[j];
			crystal_set_image(cryst, im);
			update_predictions(cryst);
		}
	}

	for (size_t i = 0; i < _pairs.size(); i++)
	{
		Reflection *ref = _pairs[i].ref;

		double fs, ss;
		get_detector_pos(ref, &fs, &ss);
		double pfs = get_temp1(ref);
		double pss = get_temp2(ref);
		double dx = fs - pfs;
		double dy = ss - pss;

		c->addDataPoint(dx, dy);
	}

	CurveView *cv = c->getCurveView();
	cv->setWindow(-10., -10., 10., 10.);
	cv->redraw();
}

void SlipPanel::updatePowder(Curve *c, bool refresh)
{
	if (refresh)
	{
		updatePeaks();
	}
	
	double slicing = 0.00005;
	double range = 0.1;
	int bins = range / slicing + 1;
	double *vals = new double[bins];
	memset(vals, '\0', sizeof(double) * bins);
	
	double min_intensity = 00;

	for (size_t k = 1; k < _imageStarts.size(); k++)
	{
		size_t start = _imageStarts[k - 1];
		size_t end = _imageStarts[k];

		for (size_t i = start + 1; i < end; i++)
		{
			if (_peaks[i].intensity < min_intensity)
			{
				continue;
			}

			for (size_t j = start; j < i; j++)
			{
				if (_peaks[j].intensity < min_intensity)
				{
					continue;
				}

				vec3 diff = make_vec3(_peaks[j].rx - _peaks[i].rx,
				                      _peaks[j].ry - _peaks[i].ry,
				                      _peaks[j].rz - _peaks[i].rz);

				double l = vec3_length(diff);
				int bin = l / slicing;

				if (bin >= bins || bin < 0) 
				{
					continue;
				}

				vals[bin]++;
			}
		}
	}

	c->clear();
	int max = 0;
	for (int i = 0; i < bins; i++)
	{
		c->addDataPoint((double)i * slicing, vals[i]);
		
		if (vals[i] > max)
		{
			max = vals[i];
		}
	}
	
	delete [] vals;
	
	CurveView *cv = c->getCurveView();
	cv->setWindow(-range/10, -(double)max/10, range, (double)max*1.1);
	cv->redraw();
}

std::string SlipPanel::shortDesc()
{
	if (_panel != NULL)
	{
		return _panel->name;
	}
	
	return "Group of " + i_to_str(_subpanels.size()) + " panels.";
}

void SlipPanel::setSelected(bool sel)
{
	_isSelected = sel;
	
	double colour = sel ? SELECTED_COLOUR: DESELECTED_COLOUR;

	for (size_t i = 0; i < _subpanels.size(); i++)
	{
		_subpanels[i]->recolour(colour, 0, 0);
	}
}

void SlipPanel::togglePanel(SlipPanel *other)
{
	std::vector<SlipPanel *>::iterator it;
	it = std::find(_subpanels.begin(), _subpanels.end(), other);
	
	if (it == _subpanels.end())
	{
		std::cout << "Pushing back" << std::endl;
		addPanel(other);
	}
	else
	{
		std::cout << "Removing" << std::endl;
		(*it)->recolour(DESELECTED_COLOUR, 0, 0);
		_subpanels.erase(it);
	}
}

void SlipPanel::clearPanels()
{
	bool tmp = _isSelected;
	setSelected(false);
	_isSelected = tmp;

	_subpanels.clear();
}