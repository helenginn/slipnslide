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

#ifndef __Slip__SlipPanel__
#define __Slip__SlipPanel__

#include "SlipObject.h"
#include "vec3.h"
#include <crystfel/detector.h>
#include <crystfel/image.h>

typedef struct
{
	Reflection *ref;
	struct imagefeature *peak;
	vec3 recip;
} RefPeak;

class Curve;

class SlipPanel : public SlipObject
{
public:
	SlipPanel(struct panel *p);
	SlipPanel();
	
	void addPanel(SlipPanel *other);	
	void togglePanel(SlipPanel *other);
	
	void clearPanels();
	
	void clearImageData()
	{
		_peaks.clear();
		_pairs.clear();
		_images.clear();
		_imageStarts.clear();
	}
	
	size_t panelCount()
	{
		return _subpanels.size();
	}
	
	std::string shortDesc();
	
	void setSelected(bool sel);

	bool isSelected()
	{
		return _isSelected;
	}

	void updateTmpPanelValues();
	void updateVertices();
	void createVertices();
	
	void getPeaksFromImage(struct image *im);
	
	void updatePowder(Curve *c, bool refresh = true);
	void updateTarget(Curve *c, bool refresh = true);
protected:
	struct imagefeature *findClosestPeak(struct image *im,
	                                     double fs, double ss);
	vec3 rayTraceToPanel(struct panel *p, vec3 dir);
	bool isValidPanelMember(struct panel *p);
	void updatePeaks();

	struct panel *panelPtr()
	{
		return _panel;
	}
private:
	vec3 _corner;      /* in pixels */
	vec3 _fs;          /* unit vector fast axis */
	vec3 _ss;          /* unit vector slow axis */
	double _width;     /* in pixels, fs multiplier */
	double _height;    /* in pixels, ss multiplier */
	double _px_per_m;  /* pixels per metre */

	bool _isSelected;
	bool _single;
	struct panel *_panel;
	std::vector<SlipPanel *> _subpanels;

	std::vector<struct imagefeature> _peaks;
	std::vector<struct image *> _images;
	std::vector<RefPeak> _pairs;
	std::vector<size_t> _imageStarts;
};

#endif