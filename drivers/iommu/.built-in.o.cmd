cmd_drivers/iommu/built-in.o :=  rm -f drivers/iommu/built-in.o; aarch64-linux-gnu-ar rcSTPD drivers/iommu/built-in.o drivers/iommu/iommu.o drivers/iommu/iommu-traces.o drivers/iommu/iommu-sysfs.o drivers/iommu/dma-iommu.o drivers/iommu/io-pgtable.o drivers/iommu/io-pgtable-arm.o drivers/iommu/iova.o drivers/iommu/of_iommu.o drivers/iommu/arm-smmu.o 