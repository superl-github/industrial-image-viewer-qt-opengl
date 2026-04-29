#pragma once
#include <QWidget>

class CyMediaRecTimeW : public QWidget {
	Q_OBJECT

public:
	explicit CyMediaRecTimeW(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
	~CyMediaRecTimeW();

public:
	void upRecTime(uint64_t time);

	void upRecTime(uint64_t saved, uint64_t sum);

	void upRecTime_Timed(uint64_t saved, uint64_t sum);
private:
	void initGui();

private:
	class PrivateData;
	PrivateData* p_data;
};