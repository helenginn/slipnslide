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

#ifndef __Slip__SlipGL__
#define __Slip__SlipGL__

#include <QtWidgets/qopenglwidget.h>
#include <QtGui/qopengl.h>
#include <QtGui/qopenglfunctions.h>
#include <crystfel/detector.h>

#include "mat3x3.h"

class SlipPanel;

class SlipGL : public QOpenGLWidget, QOpenGLFunctions
{
	Q_OBJECT
	
public:
	SlipGL(QWidget *parent, struct detector *d = NULL);
	
	void preparePanels(int n);
	void addPanel(struct panel &p);
	
	mat3x3 getModel()
	{
		return _model;
	}
public slots:
	
protected:
	virtual void initializeGL();
	virtual void paintGL();

private:
	void initialisePrograms();
	
	mat3x3 _model;
	std::vector<SlipPanel *> _panels;

	struct detector *_d;
};


#endif
