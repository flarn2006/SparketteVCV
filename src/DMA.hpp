#include "plugin.hpp"
#include <set>

namespace sparkette {
	
	template <typename T>
	class DMAChannel {
	protected:
		T *mem_start = nullptr;
		std::size_t count = 0;
		std::size_t stride = 1;
		std::size_t columns = 1;

	public:
		class accessor {
			DMAChannel *channel;
			std::size_t index;
		public:
			operator T() const { return channel->read(index); }
			accessor& operator=(T rhs) {
				channel->write(index, rhs);
				return *this;
			}
		};

		DMAChannel() = default;
		DMAChannel(T *mem_start, std::size_t count, std::size_t columns = 1, std::size_t stride = 1)
			: mem_start(mem_start), count(count), stride(stride), columns(columns) {}
		DMAChannel(const DMAChannel& other) = delete;
		DMAChannel& operator=(const DMAChannel& other) = delete;

		virtual T read(std::size_t index) const {
			return mem_start[index * stride];
		}

		T read(std::size_t col, std::size_t row) const {
			return read(columns * row + col);
		}

		virtual void write(std::size_t index, T value) {
			mem_start[index * stride] = value;
		}

		void write(std::size_t col, std::size_t row, T value) {
			write(columns * row + col, value);
		}

		std::size_t size() const {
			return count;
		}

		accessor operator[](std::size_t index) {
			accessor acc;
			acc.channel = this;
			acc.index = index;
			return acc;
		}

		T operator[](std::size_t index) const {
			return read(index);
		}
	};

	template <typename T>
	struct DMAHost {
		virtual int getDMAChannelCount() const = 0;
		virtual DMAChannel<T> *getDMAChannel(int num) = 0;
		virtual const DMAChannel<T> *getDMAChannel(int num) const {
			auto this_nonconst = const_cast<DMAHost*>(this);
			return const_cast<const DMAChannel<T>*>(this_nonconst->getDMAChannel(num));
		}
		virtual bool readyForDMA() const {
			return true;
		}
	};

	template <typename T>
	class DMAClient : public DMAHost<T> {
		DMAHost<T> *host = nullptr;

	protected:
		void setDMAHost(DMAHost<T> *newHost) {
			host = newHost;
		}

	public:
		DMAHost<T> *getDMAHost() const {
			return host;
		}

		int getDMAChannelCount() const override {
			if (host)
				return host->getDMAChannelCount();
			else
				return 0;
		}

		DMAChannel<T> *getDMAChannel(int num) override {
			if (host)
				return host->getDMAChannel(num);
			else
				return nullptr;
		}

		const DMAChannel<T> *getDMAChannel(int num) const override {
			if (host)
				return host->getDMAChannel(num);
			else
				return nullptr;
		}

		bool readyForDMA() const override {
			if (host)
				return host->readyForDMA();
			else
				return false;
		}
	};

	template <typename T>
	struct DMAExpanderModule : Module, DMAClient<T> {
		int dmaHostLightID = -1; //intended to be a GreenRedLight, so dmaHostLightID+1 will be used as well
		int dmaClientLightID = -1;

		virtual void onExpanderChange(const ExpanderChangeEvent &e) override {
			if (e.side == 0) {
				if (dmaClientLightID >= 0)
					lights[dmaClientLightID].setBrightness(dynamic_cast<DMAClient<T>*>(leftExpander.module) ? 1.f : 0.f);
			} else {
				setDMAHost(dynamic_cast<DMAHost<T>*>(rightExpander.module));
			}
		}

		virtual void process(const ProcessArgs &args) override {
			if (dmaHostLightID >= 0) {
				bool ready = this->readyForDMA();
				lights[dmaHostLightID].setBrightnessSmooth(ready ? 1.f : 0.f, args.sampleTime);
				lights[dmaHostLightID+1].setBrightnessSmooth(ready ? 0.f : 1.f, args.sampleTime);
			}
		}
	};
}
